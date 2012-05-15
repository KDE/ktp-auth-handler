/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
 * Copyright (C) 2011 David Edmundson <kde@davidedmundson.co.uk>
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

#include "x-telepathy-password-auth-operation.h"
#include "x-telepathy-password-prompt.h"

#include <KDebug>
#include <KLocalizedString>

#include <KTp/wallet-interface.h>

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
}

XTelepathyPasswordAuthOperation::~XTelepathyPasswordAuthOperation()
{
}

void XTelepathyPasswordAuthOperation::onSASLStatusChanged(uint status, const QString &reason,
        const QVariantMap &details)
{
    if (status == Tp::SASLStatusNotStarted) {
        kDebug() << "Requesting password";
        promptUser(m_canTryAgain || !KTp::WalletInterface::hasEntry(m_account, QLatin1String("lastLoginFailed")));
    } else if (status == Tp::SASLStatusServerSucceeded) {
        kDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
    } else if (status == Tp::SASLStatusSucceeded) {
        kDebug() << "Authentication succeeded";
        if (KTp::WalletInterface::hasEntry(m_account, QLatin1String("lastLoginFailed"))) {
            KTp::WalletInterface::removeEntry(m_account, QLatin1String("lastLoginFailed"));
        }
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
            KTp::WalletInterface::setEntry(m_account, QLatin1String("lastLoginFailed"), QLatin1String("true"));
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

void XTelepathyPasswordAuthOperation::promptUser(bool isFirstRun)
{
    if (KTp::WalletInterface::hasPassword(m_account) && isFirstRun) {
        m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"),
                                            KTp::WalletInterface::password(m_account).toUtf8());
    } else {
        m_dialog = new XTelepathyPasswordPrompt(m_account);
        connect(m_dialog.data(),
                SIGNAL(finished(int)),
                SLOT(onDialogFinished(int)));
        m_dialog.data()->show();
    }
}

void XTelepathyPasswordAuthOperation::onDialogFinished(int result)
{
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
        // save password in kwallet if necessary...
        if (!m_dialog.isNull()) {
            if (m_dialog.data()->savePassword()) {
                kDebug() << "Saving password in wallet";
                KTp::WalletInterface::setPassword(m_account, m_dialog.data()->password());
            }
            m_saslIface->StartMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"), m_dialog.data()->password().toUtf8());
            m_dialog.data()->deleteLater();
        }
    }
}

#include "x-telepathy-password-auth-operation.moc"
