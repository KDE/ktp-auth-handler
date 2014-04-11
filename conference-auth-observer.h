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

#ifndef CONFERENCEAUTHHANDLER_H
#define CONFERENCEAUTHHANDLER_H

#include <QObject>

#include <TelepathyQt/AbstractClientObserver>

namespace Tp
{
    class PendingOperation;
};

class ConferenceAuthObserver : public QObject, public Tp::AbstractClientObserver
{
    Q_OBJECT
public:
    explicit ConferenceAuthObserver(const Tp::ChannelClassSpecList &channelFilter);
    ~ConferenceAuthObserver();

    void observeChannel(const Tp::MethodInvocationContextPtr<> &context,
                        const Tp::AccountPtr &account,
                        const Tp::ConnectionPtr &connection,
                        const Tp::ChannelPtr &channel,
                        const Tp::ChannelDispatchOperationPtr &dispatchOperation,
                        const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                        const Tp::AbstractClientObserver::ObserverInfo &observerInfo);

private Q_SLOTS:
    void onAuthFinished(Tp::PendingOperation *op);

private:
    QHash<Tp::PendingOperation *, Tp::MethodInvocationContextPtr<> > mAuthContexts;

};

#endif // CONFERENCEAUTHHANDLER_H
