/*
 * Copyright (C) 2011 Daniele E. Domenichelli <daniele.domenichelli@gmail.com>
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

#include "x-messenger-oauth2-auth-operation.h"

#include <QDebug>

#include <KLocalizedString>
#include <TelepathyQt/Account>
#include <KTp/wallet-interface.h>

const QLatin1String XMessengerOAuth2AccessTokenWalletEntry("access_token");
const QLatin1String XMessengerOAuth2RefreshTokenWalletEntry("refresh_token");

XMessengerOAuth2AuthOperation::XMessengerOAuth2AuthOperation(
        const Tp::AccountPtr &account,
        Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface,
        KTp::WalletInterface *walletInterface) :
    Tp::PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface),
    m_walletInterface(walletInterface)
{
    // NOTE Remove old "token" wallet entry
    // This entry was used in ktp 0.3, and it is now replaced by "access_token"
    // FIXME We might want to remove this in a later ktp version
    if (m_walletInterface->hasEntry(m_account, QLatin1String("token"))) {
        m_walletInterface->removeEntry(m_account, QLatin1String("token"));
    }

    connect(m_saslIface,
            SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)),
            SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
}

XMessengerOAuth2AuthOperation::~XMessengerOAuth2AuthOperation()
{
}

void XMessengerOAuth2AuthOperation::onSASLStatusChanged(uint status, const QString &reason,
        const QVariantMap &details)
{
    switch (status) {
    case Tp::SASLStatusNotStarted :
        if (m_walletInterface->hasEntry(m_account, XMessengerOAuth2AccessTokenWalletEntry)) {
            m_saslIface->StartMechanismWithData(QLatin1String("X-MESSENGER-OAUTH2"),
                                                QByteArray::fromBase64(m_walletInterface->entry(m_account, XMessengerOAuth2AccessTokenWalletEntry).toLatin1()));
            return;
        }

        qDebug() << "Requesting password";
        m_dialog = new XMessengerOAuth2Prompt();

        connect(m_dialog.data(),
                SIGNAL(finished(int)),
                SLOT(onDialogFinished(int)));
        // Show the dialog only when the login page is loaded
        connect(m_dialog.data(),
                SIGNAL(loginPageLoaded(bool)),
                m_dialog.data(),
                SLOT(show()));
        break;
    case Tp::SASLStatusServerSucceeded:
        qDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
        break;
    case Tp::SASLStatusSucceeded:
        qDebug() << "Authentication succeeded";
        setFinished();
        break;
    case Tp::SASLStatusInProgress:
        qDebug() << "Authenticating...";
        break;
    case Tp::SASLStatusServerFailed:
    {
        qDebug() << "Error authenticating - reason:" << reason << "- details:" << details;
        if (m_walletInterface->hasEntry(m_account, XMessengerOAuth2AccessTokenWalletEntry)) {
            // We cannot try again (TODO canTryAgain seems to be always false for
            // X-MESSENGER-OAUTH but we should insert a check here)
            // but we can remove the token and request again to set the account
            // online like we do for X-TELEPATHY-PASSWORD.
            // A new channel will be created, but since we deleted the token entry,
            // next time we will prompt for password and the user won't see any
            // difference except for an authentication error notification
            // TODO There should be a way to renew the token if it is expired.
            m_walletInterface->removeEntry(m_account, XMessengerOAuth2AccessTokenWalletEntry);
            Tp::Presence requestedPresence = m_account->requestedPresence();
            m_account->setRequestedPresence(requestedPresence);
        }
        QString errorMessage = details[QLatin1String("server-message")].toString();
        setFinishedWithError(reason, errorMessage.isEmpty() ? i18n("Authentication error") : errorMessage);
        break;
    }
    default:
        qWarning() << "Unhandled status" << status;
        Q_ASSERT(false);
        break;
    }
}

void XMessengerOAuth2AuthOperation::onDialogFinished(int result)
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
        qDebug() << QLatin1String(m_dialog.data()->accessToken());
        m_walletInterface->setEntry(m_account, XMessengerOAuth2AccessTokenWalletEntry, QLatin1String(m_dialog.data()->accessToken().toBase64()));
        m_walletInterface->setEntry(m_account, XMessengerOAuth2RefreshTokenWalletEntry, QLatin1String(m_dialog.data()->refreshToken().toBase64()));
        m_saslIface->StartMechanismWithData(QLatin1String("X-MESSENGER-OAUTH2"), m_dialog.data()->accessToken());
        if (!m_dialog.isNull()) {
            m_dialog.data()->deleteLater();
        }
        return;
    default:
        Q_ASSERT(false);
        break;
    }
}

#include "x-messenger-oauth2-auth-operation.moc"
