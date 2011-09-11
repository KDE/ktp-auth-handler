/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) David Edmundson <kde@davidedmundson.co.uk>
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

#include "handler.h"

#include "handler-auth.h"

#include <QDBusConnection>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelDispatchOperation>
#include <TelepathyQt4/MethodInvocationContext>

Handler::Handler(const Tp::ChannelClassSpecList &channelFilter)
    : Tp::AbstractClientHandler(channelFilter)
{
}

Handler::~Handler()
{
}

bool Handler::bypassApproval() const
{
    return true;
}

void Handler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
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

    Q_ASSERT(channels.size() == 1);
    HandlerAuth *auth = new HandlerAuth(account, connection, channels.first());

    connect(auth,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAuthFinished(Tp::PendingOperation*)));
    mAuthContexts.insert(auth, context);

}

void Handler::onAuthFinished(Tp::PendingOperation *op)
{
    HandlerAuth *auth = qobject_cast<HandlerAuth*>(op);
    Q_ASSERT(mAuthContexts.contains(auth));

    Tp::MethodInvocationContextPtr<> context = mAuthContexts.value(auth);
    if (op->isError()) {
        context->setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        context->setFinished();
    }

    mAuthContexts.remove(auth);

    //if mAuthContexts.size == 0 then close?
}

#include "handler.moc"
