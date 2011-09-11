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

#include <QApplication>

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>

#include "handler.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    Tp::registerTypes();
//    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tp::ChannelClassSpecList channelSpecList;

    QVariantMap otherProperties;
    otherProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION ".AuthenticationMethod"),
            TP_QT4_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    channelSpecList.append(Tp::ChannelClassSpec(TP_QT4_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
                Tp::HandleTypeNone, false, otherProperties));

    // create the channel approver
    Handler handler(channelSpecList);

    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
            QDBusConnection::sessionBus(),
                Tp::Features() << Tp::Account::FeatureCore << Tp::Account::FeatureProfile);
    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
            QDBusConnection::sessionBus(), Tp::Connection::FeatureCore);
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(
            QDBusConnection::sessionBus());
    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);
    Tp::ClientRegistrarPtr clientRegistrar = Tp::ClientRegistrar::create(
            accountFactory, connectionFactory, channelFactory);

    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(&handler), QLatin1String("KDEAuthHanlder"))) {
        return 1;
    }

    return app.exec();
}
