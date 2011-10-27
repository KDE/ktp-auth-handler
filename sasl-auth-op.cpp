/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
 * Copyright (C) 2011 David Edmundson <kde@davidedmundson.co.uk>
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

#include "sasl-auth-op.h"

#include <TelepathyQt4/PendingVariantMap>

#include <KDebug>
#include <KLocalizedString>

#include "password-prompt.h"
#include "common/wallet-interface.h"

SaslAuthOp::SaslAuthOp(const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_account(account),
      m_connection(connection),
      m_channel(channel),
      m_saslIface(channel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>()),
      m_canTryAgain(false)
{
    connect(m_saslIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}

SaslAuthOp::~SaslAuthOp()
{
}

void SaslAuthOp::gotProperties(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kWarning() << "Unable to retrieve available SASL mechanisms";
        m_channel->requestClose();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
    QVariantMap props = qdbus_cast<QVariantMap>(pvm->result());
    m_canTryAgain = qdbus_cast<bool>(props.value("CanTryAgain"));
    QStringList mechanisms = qdbus_cast<QStringList>(props.value("AvailableMechanisms"));
    if (!mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        kWarning() << "X-TELEPATHY-PASSWORD is the only supported SASL mechanism and "
            "is not available";
        m_channel->requestClose();
        setFinishedWithError(TP_QT4_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD is the only supported SASL mechanism and "
                    " is not available"));
        return;
    }

    // everything ok, we can return from handleChannels now
    emit ready(this);

    connect(m_saslIface,
            SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)),
            SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
    uint status = qdbus_cast<uint>(props.value("SASLStatus"));
    QString error = qdbus_cast<QString>(props.value("SASLError"));
    QVariantMap errorDetails = qdbus_cast<QVariantMap>(props.value("SASLErrorDetails"));
    onSASLStatusChanged(status, error, errorDetails);
}

void SaslAuthOp::onSASLStatusChanged(uint status, const QString &reason,
        const QVariantMap &details)
{
    KTelepathy::WalletInterface wallet(0);
    if (status == Tp::SASLStatusNotStarted) {
        kDebug() << "Requesting password";
        promptUser (m_canTryAgain || !wallet.hasEntry(m_account, QLatin1String("lastLoginFailed")));
    } else if (status == Tp::SASLStatusServerSucceeded) {
        kDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
    } else if (status == Tp::SASLStatusSucceeded) {
        kDebug() << "Authentication succeeded";
        if (wallet.hasEntry(m_account, QLatin1String("lastLoginFailed"))) {
            wallet.removeEntry(m_account, QLatin1String("lastLoginFailed"));
        }
        m_channel->requestClose();
        setFinished();
    } else if (status == Tp::SASLStatusInProgress) {
        kDebug() << "Authenticating...";
    } else if (status == Tp::SASLStatusServerFailed) {
        kDebug() << "Error authenticating - reason:" << reason << "- details:" << details;

        if (m_canTryAgain) {
            kDebug() << "Retrying...";
            promptUser(false);
        } else {
            kWarning() << "Authentication failed and cannot try again";
            wallet.setEntry(m_account, QLatin1String("lastLoginFailed"), QLatin1String("true"));
            m_channel->requestClose();
            QString errorMessage = details[QLatin1String("server-message")].toString();
            setFinishedWithError(reason, errorMessage.isEmpty() ? i18n("Authentication error") : errorMessage);
        }
    }
}

void SaslAuthOp::promptUser(bool isFirstRun)
{
    QString password;

    kDebug() << "Trying to load from wallet";
    KTelepathy::WalletInterface wallet(0);
    if (wallet.hasPassword(m_account) && isFirstRun) {
        password = wallet.password(m_account);
    } else {
        PasswordPrompt dialog(m_account);
        if (dialog.exec() == QDialog::Rejected) {
            kDebug() << "Authentication cancelled";
            m_saslIface->AbortSASL(Tp::SASLAbortReasonUserAbort, "User cancelled auth");
            m_channel->requestClose();
            setFinished();
            return;
        }
        password = dialog.password();
        // save password in kwallet if necessary...
        if (dialog.savePassword()) {
            kDebug() << "Saving password in wallet";
            wallet.setPassword(m_account, dialog.password());
        }
    }

    m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), password.toUtf8());
}

#include "sasl-auth-op.moc"
