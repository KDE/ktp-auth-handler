/*
 * Copyright (C) 2012 Rohan Garg <rohangarg@kubuntu.org>
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

#include "conference-auth-op.h"
#include "x-telepathy-password-auth-operation.h"

#include <TelepathyQt/PendingVariantMap>

#include <QDebug>

#include <KLocalizedString>
#include <KPasswordDialog>

#include <KTp/wallet-interface.h>
#include <KTp/pending-wallet.h>

ConferenceAuthOp::ConferenceAuthOp(const Tp::AccountPtr &account,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_walletInterface(0),
      m_account(account),
      m_channel(channel),
      m_passwordIface(channel->interface<Tp::Client::ChannelInterfacePasswordInterface>())
{
    connect(KTp::WalletInterface::openWallet(), SIGNAL(finished(Tp::PendingOperation*)), SLOT(onOpenWalletOperationFinished(Tp::PendingOperation*)));
}

ConferenceAuthOp::~ConferenceAuthOp()
{
}

void ConferenceAuthOp::onOpenWalletOperationFinished(Tp::PendingOperation *op)
{
    KTp::PendingWallet *walletOp = qobject_cast<KTp::PendingWallet*>(op);
    Q_ASSERT(walletOp);

    m_walletInterface = walletOp->walletInterface();

    qDebug() << "Wallet is open :" << m_walletInterface->isOpen();

    QDBusPendingReply<uint> reply = m_passwordIface->GetPasswordFlags();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(passwordFlagOperationFinished(QDBusPendingCallWatcher*)));

}

void ConferenceAuthOp::passwordFlagOperationFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Reply is a error. ABORT!";
        return;
    }


    if (reply.argumentAt<0>() == Tp::ChannelPasswordFlagProvide) {
        if (m_walletInterface->hasEntry(m_account, m_channel->targetId())) {
            providePassword(m_walletInterface->entry(m_account, m_channel->targetId()));
        } else {
            passwordDialog();
        }
    }
}

void ConferenceAuthOp::providePassword(const QString &password)
{
    m_password = password;
    QDBusPendingReply<bool> reply = m_passwordIface->ProvidePassword(m_password);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect (watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
             SLOT(onPasswordProvided(QDBusPendingCallWatcher*)));
}

void ConferenceAuthOp::passwordDialog()
{
    KPasswordDialog *passwordDialog = new KPasswordDialog;
    passwordDialog->setAttribute(Qt::WA_DeleteOnClose);
    passwordDialog->setPrompt(i18n("Please provide a password for the chat room %1", m_channel->targetId()));
    passwordDialog->show();

    connect(passwordDialog, SIGNAL(gotPassword(QString,bool)),
            SLOT(providePassword(QString)));

}

void ConferenceAuthOp::onPasswordProvided(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool> reply = *watcher;
    if (!reply.isValid() || reply.count() < 1) {
        return;
    }

    if (reply.argumentAt<0>()) {
        m_walletInterface->setEntry(m_account,m_channel->targetId(), m_password);
        setFinished();
    } else {
        qDebug() << "Password was incorrect, enter again";
        passwordDialog();
    }
}

#include "conference-auth-op.moc"
