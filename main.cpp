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
#include "conference-auth-observer.h"
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
    aboutData.setProductName("telepathy/auth-handler");
    aboutData.setProgramIconName(QLatin1String("telepathy-kde"));

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

    QMap<QString,Tp::AbstractClientPtr> handlers;
    handlers.insert(QLatin1String("KTp.SASLHandler"), Tp::SharedPtr<SaslHandler>(new SaslHandler));
    handlers.insert(QLatin1String("KTp.TLSHandler"), Tp::SharedPtr<TlsHandler>(new TlsHandler));
    Tp::ChannelClassSpecList confAuthFilter =  Tp::ChannelClassSpecList() << Tp::ChannelClassSpec::textChatroom();
    handlers.insert(QLatin1String("KTp.ConfAuthObserver"), Tp::SharedPtr<ConferenceAuthObserver>(new ConferenceAuthObserver(confAuthFilter)));

    int loadedHandlers = handlers.count();
    QMap<QString,Tp::AbstractClientPtr>::ConstIterator iter = handlers.constBegin();
    for (; iter != handlers.constEnd(); ++iter) {
        if (!clientRegistrar->registerClient(iter.value(), iter.key())) {
            loadedHandlers--;
        }
    }

    if (loadedHandlers == 0) {
        kDebug() << "No handlers registered. Exiting";
        return 1;
    }

    return app.exec();
}
