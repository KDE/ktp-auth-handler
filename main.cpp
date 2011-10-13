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

#include <KAboutData>
#include <KCmdLineArgs>
#include <KApplication>
#include <KDebug>

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>

#include "sasl-handler.h"
#include "tls-handler.h"
#include "common/telepathy-handler-application.h"

// FIXME: Move this to tp-qt4 itself
#include "types.h"

int main(int argc, char *argv[])
{
    KAboutData aboutData("telepathy-kde-auth-handler",
                         0,
                         ki18n("Telepathy Authentication Handler"),
                         "0.1");
    aboutData.addAuthor(ki18n("David Edmundson"), ki18n("Developer"), "kde@davidedmundson.co.uk");
    aboutData.setProductName("telepathy/auth-handler");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KTelepathy::TelepathyHandlerApplication app;

    // FIXME: Move this to tp-qt4 itself
    registerTypes();

    Tp::AccountFactoryPtr accountFactory = Tp::AccountFactory::create(
            QDBusConnection::sessionBus(), Tp::Account::FeatureCore);
    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(
            QDBusConnection::sessionBus(), Tp::Connection::FeatureCore);
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(
            QDBusConnection::sessionBus());
    channelFactory->addCommonFeatures(Tp::Channel::FeatureCore);
    Tp::ClientRegistrarPtr clientRegistrar = Tp::ClientRegistrar::create(
            accountFactory, connectionFactory, channelFactory);

    int handlers = 2;

    Tp::ChannelClassSpecList saslFilter;
    QVariantMap saslOtherProperties;
    saslOtherProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION ".AuthenticationMethod"),
            TP_QT4_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    saslFilter.append(Tp::ChannelClassSpec(TP_QT4_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
                Tp::HandleTypeNone, false, saslOtherProperties));
    Tp::SharedPtr<SaslHandler> saslHandler = Tp::SharedPtr<SaslHandler>(new SaslHandler(saslFilter));
    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(saslHandler), QLatin1String("KDE.SASL.Handler"))) {
        handlers -= 1;
    }

    Tp::ChannelClassSpecList tlsFilter;
    tlsFilter.append(Tp::ChannelClassSpec(TP_QT4_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION,
                Tp::HandleTypeNone, false));
    Tp::SharedPtr<TlsHandler> tlsHandler = Tp::SharedPtr<TlsHandler>(new TlsHandler(tlsFilter));
    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(tlsHandler), QLatin1String("KDE.TLS.Handler"))) {
        handlers -= 1;
    }

    if (!handlers)
    {
        kDebug() << "No handlers registered. Exiting";
        return 1;
    }
    return app.exec();
}
