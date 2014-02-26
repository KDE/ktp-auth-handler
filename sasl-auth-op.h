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

#ifndef SASL_AUTH_OP_H
#define SASL_AUTH_OP_H

#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

namespace KTp {
    class WalletInterface;
}

class SaslAuthOp : public Tp::PendingOperation
{
    Q_OBJECT

public:
    SaslAuthOp(const Tp::AccountPtr &account,
            const Tp::ChannelPtr &channel);
    ~SaslAuthOp();

Q_SIGNALS:
    void ready(Tp::PendingOperation *self);

private Q_SLOTS:
    void gotProperties(Tp::PendingOperation *op);
    void onOpenWalletOperationFinished(Tp::PendingOperation *op);
    void onAuthOperationFinished(Tp::PendingOperation *op);

#ifdef HAVE_SSO
    //FIXME this is a workaround until Tp::Client::AccountInterfaceStorageInterface is merged into Tp::Account
    //https://bugs.freedesktop.org/show_bug.cgi?id=63191
    void onGetAccountStorageFetched(Tp::PendingOperation *op);
#endif

private:
    void setReady();
    KTp::WalletInterface *m_walletInterface;
    Tp::AccountPtr m_account;
    Tp::ChannelPtr m_channel;
    Tp::Client::ChannelInterfaceSASLAuthentication1Interface *m_saslIface;

#ifdef HAVE_SSO
    void fetchAccountStorage();
    int m_accountStorageId;
#endif
};

#endif // SASL_AUTH_OP_H
