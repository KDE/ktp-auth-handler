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

#ifndef SASLHANDLER_H
#define SASLHANDLER_H

#include <QObject>

#include <TelepathyQt/AbstractClientHandler>

namespace Tp
{
    class PendingOperation;
};

class SaslAuthOp;

class SaslHandler : public QObject, public Tp::AbstractClientHandler
{
    Q_OBJECT

public:
    explicit SaslHandler(const Tp::ChannelClassSpecList &channelFilter);
    ~SaslHandler();

    bool bypassApproval() const;

    void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
            const Tp::AccountPtr &account,
            const Tp::ConnectionPtr &connection,
            const QList<Tp::ChannelPtr> &channels,
            const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

private Q_SLOTS:
    void onAuthReady(Tp::PendingOperation *op);
    void onAuthFinished(Tp::PendingOperation *op);

private:
    QHash<Tp::PendingOperation *, Tp::MethodInvocationContextPtr<> > mAuthContexts;
};

#endif
