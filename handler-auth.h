/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_examples_saslauth_handler_auth_h_HEADER_GUARD_
#define _TelepathyQt4_examples_saslauth_handler_auth_h_HEADER_GUARD_

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Types>

#include "KWallet/Wallet"

#include "sasl-channel.h"

class HandlerAuth : public Tp::PendingOperation
{
    Q_OBJECT
public:
    HandlerAuth(const Tp::AccountPtr &account,
            const Tp::ConnectionPtr &connection,
            const Tp::ChannelPtr &channel);
    ~HandlerAuth();

private Q_SLOTS:
    void gotAvailableSASLMechanisms(Tp::PendingOperation *op);
    void onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details);
    void promptUser(bool isFirstPrompt);

private:
    Tp::AccountPtr m_account;
    Tp::ConnectionPtr m_connection;
    SaslChannel *m_channel; //FIXME
    KWallet::Wallet *m_wallet;
};

#endif
