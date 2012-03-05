/*
 * Copyright (C) 2011-2012 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
 *   @author Jeremy Whiting <jeremy.whiting@collabora.co.uk>
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

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>

#include "sasl-handler.h"
#include "tls-handler.h"
#include "captcha-handler.h"
#include "version.h"

#include <KTp/telepathy-handler-application.h>

// FIXME: Move this to tp-qt4 itself
#include "types.h"

int main(int argc, char *argv[])
{
    KAboutData aboutData("ktp-auth-handler", 0,
                         ki18n("Telepathy Authentication Handler"),
                         KTP_AUTH_HANDLER_VERSION);
    aboutData.addAuthor(ki18n("David Edmundson"), ki18n("Developer"), "kde@davidedmundson.co.uk");
    aboutData.addAuthor(ki18n("Daniele E. Domenichelli"), ki18n("Developer"), "daniele.domenichelli@gmail.com");
    aboutData.addAuthor(ki18n("Jeremy Whiting"), ki18n("Developer"), "jpwhiting@kde.org");
    aboutData.setProductName("telepathy/auth-handler");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KTp::TelepathyHandlerApplication app;

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
            TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + QLatin1String(".AuthenticationMethod"),
            TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    saslFilter.append(Tp::ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
                Tp::HandleTypeNone, false, saslOtherProperties));
    Tp::SharedPtr<SaslHandler> saslHandler = Tp::SharedPtr<SaslHandler>(new SaslHandler(saslFilter));
    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(saslHandler), QLatin1String("KTp.SASLHandler"))) {
        handlers -= 1;
    }

    Tp::SharedPtr<CaptchaHandler> captchaHandler = Tp::SharedPtr<CaptchaHandler>(new CaptchaHandler());
    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(captchaHandler), QLatin1String("KTp.CaptchaHandler"))) {
        handlers -= 1;
        kDebug() << "failed to register captcha handler";
    }

#if 0
    Tp::ChannelClassSpecList tlsFilter;
    tlsFilter.append(Tp::ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION,
                Tp::HandleTypeNone, false));
    Tp::SharedPtr<TlsHandler> tlsHandler = Tp::SharedPtr<TlsHandler>(new TlsHandler(tlsFilter));
    if (!clientRegistrar->registerClient(
                Tp::AbstractClientPtr(tlsHandler), QLatin1String("KTp.TLSHandler"))) {
        handlers -= 1;
    }
#endif

    if (!handlers) {
        kDebug() << "No handlers registered. Exiting";
        return 1;
    }
    return app.exec();
}
