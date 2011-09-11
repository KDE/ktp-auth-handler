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

#ifndef _TelepathyQt4_examples_saslauth_handler_h_HEADER_GUARD_
#define _TelepathyQt4_examples_saslauth_handler_h_HEADER_GUARD_

#include <QObject>

#include <TelepathyQt4/AbstractClientHandler>

namespace Tp
{
    class PendingOperation;
};

class HandlerAuth;

class Handler : public QObject, public Tp::AbstractClientHandler
{
    Q_OBJECT

public:
    explicit Handler(const Tp::ChannelClassSpecList &channelFilter);
    ~Handler();

    bool bypassApproval() const;

    void handleChannels(const Tp::MethodInvocationContextPtr<> &context,
            const Tp::AccountPtr &account,
            const Tp::ConnectionPtr &connection,
            const QList<Tp::ChannelPtr> &channels,
            const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const Tp::AbstractClientHandler::HandlerInfo &handlerInfo);

private Q_SLOTS:
    void onAuthFinished(Tp::PendingOperation *op);

private:
    QHash<HandlerAuth *, Tp::MethodInvocationContextPtr<> > mAuthContexts;
};

#endif
