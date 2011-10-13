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

#include "sasl-handler.h"

#include "sasl-auth-op.h"
#include "common/telepathy-handler-application.h"

#include <QDBusConnection>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelDispatchOperation>
#include <TelepathyQt4/MethodInvocationContext>

#include <KDebug>

SaslHandler::SaslHandler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter)
{
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
            0, KWallet::Wallet::Asynchronous);
}

SaslHandler::~SaslHandler()
{
}

bool SaslHandler::bypassApproval() const
{
    return true;
}

void SaslHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
        const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const QList<Tp::ChannelPtr> &channels,
        const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const Tp::AbstractClientHandler::HandlerInfo &handlerInfo)
{
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    KTelepathy::TelepathyHandlerApplication::newJob();
    SaslAuthOp *auth = new SaslAuthOp(
            account, connection, channels.first(), m_wallet);
    connect(auth,
            SIGNAL(ready(Tp::PendingOperation*)),
            SLOT(onAuthReady(Tp::PendingOperation*)));
    connect(auth,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAuthFinished(Tp::PendingOperation*)));
    mAuthContexts.insert(auth, context);
}

void SaslHandler::onAuthReady(Tp::PendingOperation *op)
{
    SaslAuthOp *auth = qobject_cast<SaslAuthOp*>(op);
    Q_ASSERT(mAuthContexts.contains(auth));

    Tp::MethodInvocationContextPtr<> context = mAuthContexts.value(auth);
    context->setFinished();
}

void SaslHandler::onAuthFinished(Tp::PendingOperation *op)
{
    SaslAuthOp *auth = qobject_cast<SaslAuthOp*>(op);
    Q_ASSERT(mAuthContexts.contains(auth));

    if (op->isError()) {
        kWarning() << "Error in SASL auth:" << op->errorName() << "-" << op->errorMessage();
    }

    mAuthContexts.remove(auth);
    KTelepathy::TelepathyHandlerApplication::jobFinished();
}

#include "sasl-handler.moc"
