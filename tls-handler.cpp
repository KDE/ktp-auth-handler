/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
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

#include "tls-handler.h"

#include "tls-cert-verifier-op.h"

#include <KTp/telepathy-handler-application.h>

#include <QDBusConnection>

#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/MethodInvocationContext>

#include <KDebug>

static inline Tp::ChannelClassSpecList channelFilter() {
    Tp::ChannelClassSpecList filter;
    filter.append(Tp::ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION,
                Tp::HandleTypeNone, false));
    return filter;
}

TlsHandler::TlsHandler()
    : Tp::AbstractClientHandler(channelFilter())
{
}

TlsHandler::~TlsHandler()
{
}

bool TlsHandler::bypassApproval() const
{
    return true;
}

void TlsHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
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

    KTp::TelepathyHandlerApplication::newJob();
    TlsCertVerifierOp *verifier = new TlsCertVerifierOp(
            account, connection, channels.first());
    connect(verifier,
            SIGNAL(ready(Tp::PendingOperation*)),
            SLOT(onCertVerifierReady(Tp::PendingOperation*)));
    connect(verifier,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onCertVerifierFinished(Tp::PendingOperation*)));
    mVerifiers.insert(verifier, context);
}

void TlsHandler::onCertVerifierReady(Tp::PendingOperation *op)
{
    TlsCertVerifierOp *verifier = qobject_cast<TlsCertVerifierOp*>(op);
    Q_ASSERT(mVerifiers.contains(verifier));

    Tp::MethodInvocationContextPtr<> context = mVerifiers.value(verifier);
    context->setFinished();
}

void TlsHandler::onCertVerifierFinished(Tp::PendingOperation *op)
{
    TlsCertVerifierOp *verifier = qobject_cast<TlsCertVerifierOp*>(op);
    Q_ASSERT(mVerifiers.contains(verifier));

    if (op->isError()) {
        kWarning() << "Error verifying TLS certificate:" << op->errorName() << "-" << op->errorMessage();
    }

    mVerifiers.remove(verifier);
    KTp::TelepathyHandlerApplication::jobFinished();
}

#include "tls-handler.moc"
