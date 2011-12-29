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

#include <KUrl>
#include <KDebug>
#include <KLocalizedString>
#include <TelepathyQt/Account>
#include <KTp/wallet-interface.h>

const QLatin1String XMessengerOAuth2TokenWalletEntry("token");

XMessengerOAuth2AuthOperation::XMessengerOAuth2AuthOperation(
        const Tp::AccountPtr &account,
        Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface) :
    Tp::PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface)
{
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
    KTp::WalletInterface wallet(0);

    switch (status) {
    case Tp::SASLStatusNotStarted :
        if (wallet.hasEntry(m_account, XMessengerOAuth2TokenWalletEntry)) {
            m_saslIface->StartMechanismWithData(QLatin1String("X-MESSENGER-OAUTH2"),
                                                QByteArray::fromBase64(wallet.entry(m_account, XMessengerOAuth2TokenWalletEntry).toLatin1()));
            return;
        }

        kDebug() << "Requesting password";
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
        kDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
        break;
    case Tp::SASLStatusSucceeded:
        kDebug() << "Authentication succeeded";
        setFinished();
        break;
    case Tp::SASLStatusInProgress:
        kDebug() << "Authenticating...";
        break;
    case Tp::SASLStatusServerFailed:
    {
        kDebug() << "Error authenticating - reason:" << reason << "- details:" << details;
        QString errorMessage = details[QLatin1String("server-message")].toString();
        setFinishedWithError(reason, errorMessage.isEmpty() ? i18n("Authentication error") : errorMessage);
        break;
    }
    default:
        kWarning() << "Unhandled status" << status;
        Q_ASSERT(false);
        break;
    }
}

void XMessengerOAuth2AuthOperation::onDialogFinished(int result)
{
    KTp::WalletInterface wallet(0);
    switch (result) {
    case QDialog::Rejected:
        kDebug() << "Authentication cancelled";
        m_saslIface->AbortSASL(Tp::SASLAbortReasonUserAbort, i18n("User cancelled auth"));
        setFinished();
        if (!m_dialog.isNull()) {
            m_dialog.data()->deleteLater();
        }
        return;
    case QDialog::Accepted:
        kDebug() << QLatin1String(m_dialog.data()->accessToken());
        wallet.setEntry(m_account, XMessengerOAuth2TokenWalletEntry, m_dialog.data()->accessToken().toBase64());
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
