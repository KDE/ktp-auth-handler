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

#ifndef CONFERENCEAUTHOP_H
#define CONFERENCEAUTHOP_H

#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

namespace KTp {
    class WalletInterface;
}

class ConferenceAuthOp : public Tp::PendingOperation
{
    Q_OBJECT
public:
    ConferenceAuthOp(const Tp::AccountPtr &account,
                     const Tp::ChannelPtr &channel);
    ~ConferenceAuthOp();

private Q_SLOTS:
    void onOpenWalletOperationFinished(Tp::PendingOperation *op);
    void providePassword(const QString &password);
    void passwordFlagOperationFinished(QDBusPendingCallWatcher* watcher);
    void onPasswordProvided(QDBusPendingCallWatcher* watcher);

private:
    KTp::WalletInterface *m_walletInterface;
    Tp::AccountPtr m_account;
    Tp::ChannelPtr m_channel;
    Tp::Client::ChannelInterfacePassword1Interface *m_passwordIface;
    QString m_password;
    void passwordDialog();

};

#endif // CONFERENCEAUTHOP_H
