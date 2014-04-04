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

#include <KTp/telepathy-handler-application.h>

#include <QDBusConnection>

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/MethodInvocationContext>

#include <KDebug>

static inline Tp::ChannelClassSpecList channelFilter() {
    Tp::ChannelClassSpecList filter;
    QVariantMap saslOtherProperties;
    saslOtherProperties.insert(
            TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION1 + QLatin1String(".AuthenticationMethod"),
            TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION1);
    filter.append(Tp::ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION1,
                Tp::EntityTypeNone, false, saslOtherProperties));
    return filter;
}


SaslHandler::SaslHandler()
    : Tp::AbstractClientHandler(channelFilter())
{
}

SaslHandler::~SaslHandler()
{
}

bool SaslHandler::bypassApproval() const
{
    return true;
}

void SaslHandler::handleChannel(const Tp::MethodInvocationContextPtr<> &context,
        const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const Tp::ChannelPtr &channel,
        const QVariantMap &channelProperties,
        const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
        const QDateTime &userActionTime,
        const Tp::AbstractClientHandler::HandlerInfo &handlerInfo)
{
    Q_UNUSED(connection);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);
    Q_UNUSED(channelProperties);

    KTp::TelepathyHandlerApplication::newJob();
    SaslAuthOp *auth = new SaslAuthOp(
            account, channel);
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
    KTp::TelepathyHandlerApplication::jobFinished();
}

#include "sasl-handler.moc"
