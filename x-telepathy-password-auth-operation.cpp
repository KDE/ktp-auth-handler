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
        Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface,
        bool canTryAgain) :
    Tp::PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface),
    m_canTryAgain(canTryAgain)
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
                m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), qobject_cast<GetCredentialsJob*>(job)->credentialsData().value("Secret").toByteArray());
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
        setFinished();
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
                Accounts::Manager *manager = KAccounts::accountsManager();
                if (m_kaccountsId == 0) {
                    //we don't have KAccounts account yet, let's try to add it
                    QString username = m_account->parameters().value(QStringLiteral("account")).toString();
                    SignOn::IdentityInfo info;
                    info.setUserName(username);
                    info.setSecret(m_dialog->password());
                    info.setCaption(username);
                    info.setAccessControlList(QStringList(QLatin1String("*")));
                    info.setType(SignOn::IdentityInfo::Application);

                    SignOn::Identity *identity = SignOn::Identity::newIdentity(info, this);
                    identity->storeCredentials();

                    QString providerName = QStringLiteral("ktp-");

                    providerName.append(m_account->serviceName());

                    qDebug() << "Creating account with providerName" << providerName;

                    Accounts::Account *account = manager->createAccount(providerName);
                    account->setDisplayName(m_account->displayName());
                    account->setCredentialsId(info.id());
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

                    connect(account, &Accounts::Account::synced, [=]() {
                        m_kaccountsId = account->id();

                        QString uid = m_account->objectPath();

                        KSharedConfigPtr kaccountsConfig = KSharedConfig::openConfig(QStringLiteral("kaccounts-ktprc"));
                        KConfigGroup ktpKaccountsGroup = kaccountsConfig->group(QStringLiteral("ktp-kaccounts"));
                        ktpKaccountsGroup.writeEntry(uid, account->id());

                        KConfigGroup kaccountsKtpGroup = kaccountsConfig->group(QStringLiteral("kaccounts-ktp"));
                        kaccountsKtpGroup.writeEntry(QString::number(account->id()), uid);
                        qDebug() << "Syncing config";
                        kaccountsConfig->sync();
                    });

                    account->sync();
                } else {
                    Accounts::Account *acc = manager->account(m_kaccountsId);
                    if (acc) {
                        Accounts::AccountService *service = new Accounts::AccountService(acc, manager->service(QString()), m_dialog);
                        Accounts::AuthData authData = service->authData();
                        SignOn::Identity *identity = SignOn::Identity::existingIdentity(authData.credentialsId(), m_dialog);

                        // Get the current identity info, change the password and store it back
                        connect(identity, &SignOn::Identity::info, [this, identity](SignOn::IdentityInfo info){
                            info.setSecret(m_dialog->password());
                            identity->storeCredentials(info);
                            // we don't need the dialog anymore, delete it
                            m_dialog.data()->deleteLater();
                        });
                        identity->queryInfo();
                    } else {
                        // FIXME: Should this show a message box to the user? Or a notification?
                        qWarning() << "Could not open Accounts Manager, password will not be stored";
                        m_dialog.data()->deleteLater();
                    }
                }
            } else {
                // The user does not want to save the password, delete the dialog
                m_dialog.data()->deleteLater();
            }
            m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), m_dialog.data()->password().toUtf8());
        }
    }
}

#include "x-telepathy-password-auth-operation.moc"
