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

#ifndef X_MESSENGER_OAUTH2_AUTH_OPERATION_H
#define X_MESSENGER_OAUTH2_AUTH_OPERATION_H

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Types>

#include "x-messenger-oauth2-prompt.h"

namespace KTp
{
    class WalletInterface;
}

class XMessengerOAuth2AuthOperation : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(XMessengerOAuth2AuthOperation)

public:
    explicit XMessengerOAuth2AuthOperation(
            const Tp::AccountPtr &account,
            Tp::Client::ChannelInterfaceSASLAuthentication1Interface *saslIface,
            KTp::WalletInterface *walletInterface);
    ~XMessengerOAuth2AuthOperation();

private Q_SLOTS:
    void onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details);
    void onDialogFinished(int result);

private:
    Tp::AccountPtr m_account;
    Tp::Client::ChannelInterfaceSASLAuthentication1Interface *m_saslIface;
    KTp::WalletInterface *m_walletInterface;
    QWeakPointer<XMessengerOAuth2Prompt> m_dialog;

    friend class SaslAuthOp;
};


#endif // X_MESSENGER_OAUTH2_AUTH_OPERATION_H
