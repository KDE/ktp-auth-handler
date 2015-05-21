/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
 * Copyright (C) 2011 David Edmundson <kde@davidedmundson.co.uk>
 * Copyright (C) 2011 Daniele E. Domenichelli <daniele.domenichelli@gmail.com>
 * Copyright (C) 2014 Martin Klapetek <mklapetek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "x-telepathy-password-auth-operation.h"
#include "x-telepathy-password-prompt.h"

#include <KAccounts/getcredentialsjob.h>
#include <KAccounts/core.h>

#include <QDebug>

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/AccountService>
#include <SignOn/Identity>

XTelepathyPasswordAuthOperation::XTelepathyPasswordAuthOperation(
        const Tp::AccountPtr &account,
        int accountStorageId,
        Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface,
        bool canTryAgain) :
    Tp::PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface),
    m_canTryAgain(canTryAgain),
    m_canFinish(false),
    m_accountStorageId(accountStorageId)
{
    connect(m_saslIface,
            SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)),
            SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));

    m_config = KSharedConfig::openConfig(QStringLiteral("kaccounts-ktprc"));
    m_lastLoginFailedConfig = m_config->group(QStringLiteral("lastLoginFailed"));

    KConfigGroup kaccountsGroup = m_config->group(QStringLiteral("ktp-kaccounts"));
    m_kaccountsId = kaccountsGroup.readEntry(m_account->objectPath(), 0);
}

XTelepathyPasswordAuthOperation::~XTelepathyPasswordAuthOperation()
{
}

void XTelepathyPasswordAuthOperation::onSASLStatusChanged(uint status, const QString &reason,
        const QVariantMap &details)
{
    if (status == Tp::SASLStatusNotStarted) {
        qDebug() << "Requesting password";
        // if we have non-null id AND if the last attempt didn't fail,
        // proceed with the credentials receieved from the SSO;
        // otherwise prompt the user
        if (m_kaccountsId != 0 && !m_lastLoginFailedConfig.hasKey(m_account->objectPath())) {
            GetCredentialsJob *credentialsJob = new GetCredentialsJob(m_kaccountsId, this);
            connect(credentialsJob, &GetCredentialsJob::finished, [this](KJob *job){
                if (job->error()) {
                    qWarning() << "Credentials job error:" << job->errorText();
                    qDebug() << "Prompting for password";
                    promptUser();
                } else {
                    m_canFinish = true;
                    QByteArray secret = qobject_cast<GetCredentialsJob*>(job)->credentialsData().value("Secret").toByteArray();
                    m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), secret);
                }
            });
            credentialsJob->start();
        } else {
            promptUser();
        }
    } else if (status == Tp::SASLStatusServerSucceeded) {
        qDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
    } else if (status == Tp::SASLStatusSucceeded) {
        qDebug() << "Authentication succeeded";
        if (m_lastLoginFailedConfig.hasKey(m_account->objectPath())) {
            m_lastLoginFailedConfig.deleteEntry(m_account->objectPath());
        }
        if (m_canFinish) {
            // if the credentials storage has finished, just finish
            setFinished();
        } else {
            // ...otherwise set this to true and it will finish
            // when credentials are finished
            m_canFinish = true;
        }
    } else if (status == Tp::SASLStatusInProgress) {
        qDebug() << "Authenticating...";
    } else if (status == Tp::SASLStatusServerFailed) {
        qDebug() << "Error authenticating - reason:" << reason << "- details:" << details;

        if (m_canTryAgain) {
            qDebug() << "Retrying...";
            promptUser();
        } else {
            qWarning() << "Authentication failed and cannot try again";
            m_lastLoginFailedConfig.writeEntry(m_account->objectPath(), "1");

            // We cannot try again, but we can request again to set the account
            // online. A new channel will be created, but since we set the
            // lastLoginFailed entry, next time we will prompt for password
            // and the user won't see any difference except for an
            // authentication error notification
            Tp::Presence requestedPresence = m_account->requestedPresence();
            m_account->setRequestedPresence(requestedPresence);
            QString errorMessage = details[QLatin1String("server-message")].toString();
            setFinishedWithError(reason, errorMessage.isEmpty() ? i18n("Authentication error") : errorMessage);
        }
    }
}

void XTelepathyPasswordAuthOperation::promptUser()
{
    m_dialog = new XTelepathyPasswordPrompt(m_account);
    connect(m_dialog.data(),
            SIGNAL(finished(int)),
            SLOT(onDialogFinished(int)));
    m_dialog.data()->show();
}

void XTelepathyPasswordAuthOperation::onDialogFinished(int result)
{
    switch (result) {
    case QDialog::Rejected:
        qDebug() << "Authentication cancelled";
        m_saslIface->AbortSASL(Tp::SASLAbortReasonUserAbort, i18n("User cancelled auth"));
        setFinished();
        if (!m_dialog.isNull()) {
            m_dialog.data()->deleteLater();
        }
        return;
    case QDialog::Accepted:
        // save password in kwallet if necessary...
        if (!m_dialog.isNull()) {
            if (m_dialog.data()->savePassword()) {
                qDebug() << "Saving password in SSO";
                m_canFinish = false;
                storeCredentials(m_dialog.data()->password());
            } else {
                m_canFinish = true;
            }

            m_dialog.data()->deleteLater();

            m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), m_dialog.data()->password().toUtf8());
        }
    }
}

void XTelepathyPasswordAuthOperation::storeCredentials(const QString &secret)
{
    QString username = m_account->parameters().value(QStringLiteral("account")).toString();
    Accounts::Manager *manager = KAccounts::accountsManager();
    Accounts::Account *account = manager->account(m_kaccountsId);
    SignOn::Identity *identity;

    if (account) {
        Accounts::AccountService *service = new Accounts::AccountService(account, manager->service(QString()), this);
        Accounts::AuthData authData = service->authData();
        identity = SignOn::Identity::existingIdentity(authData.credentialsId(), this);
    } else {
        // there's no valid KAccounts account, so let's try creating one
        QString providerName = QStringLiteral("ktp-");

        providerName.append(m_account->serviceName());

        qDebug() << "Creating account with providerName" << providerName;

        account = manager->createAccount(providerName);
        account->setDisplayName(m_account->displayName());
        account->setValue("uid", m_account->objectPath());
        account->setValue("username", username);
        account->setValue(QStringLiteral("auth/mechanism"), QStringLiteral("password"));
        account->setValue(QStringLiteral("auth/method"), QStringLiteral("password"));

        account->setEnabled(true);

        Accounts::ServiceList services = account->services();
        Q_FOREACH(const Accounts::Service &service, services) {
            account->selectService(service);
            account->setEnabled(true);
        }
    }

    SignOn::IdentityInfo info;
    info.setUserName(username);
    info.setSecret(secret);
    info.setCaption(username);
    info.setAccessControlList(QStringList(QLatin1String("*")));
    info.setType(SignOn::IdentityInfo::Application);

    if (!identity) {
        // we don't have a valid SignOn::Identity, let's create new one
        identity = SignOn::Identity::newIdentity(info, this);
    }

    identity->storeCredentials(info);

    connect(identity, &SignOn::Identity::credentialsStored, [=](const quint32 id) {
        account->setCredentialsId(id);
        account->sync();

        connect(account, &Accounts::Account::synced, [=]() {
            m_kaccountsId = account->id();

            QString uid = m_account->objectPath();

            KSharedConfigPtr kaccountsConfig = KSharedConfig::openConfig(QStringLiteral("kaccounts-ktprc"));
            KConfigGroup ktpKaccountsGroup = kaccountsConfig->group(QStringLiteral("ktp-kaccounts"));
            ktpKaccountsGroup.writeEntry(uid, account->id());

            KConfigGroup kaccountsKtpGroup = kaccountsConfig->group(QStringLiteral("kaccounts-ktp"));
            kaccountsKtpGroup.writeEntry(QString::number(account->id()), uid);
            kaccountsConfig->sync();

            qDebug() << "Account credentials synchronisation finished";

            if (m_canFinish) {
                // if the sasl channel has already finished, set finished
                setFinished();
            } else {
                // ...otherwise set this to true and when the auth succeeds,
                // it will finish then
                m_canFinish = true;
            }
        });
    });
}
