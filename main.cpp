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
#include <KLocalizedString>

#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QIcon>

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
    KAboutData aboutData("ktp-auth-handler",
                         i18n("Telepathy Authentication Handler"),

                         KTP_AUTH_HANDLER_VERSION);
    aboutData.addAuthor(i18n("David Edmundson"), i18n("Developer"), "kde@davidedmundson.co.uk");
    aboutData.addAuthor(i18n("Daniele E. Domenichelli"), i18n("Developer"), "daniele.domenichelli@gmail.com");
    aboutData.setProductName("telepathy/auth-handler");

    KAboutData::setApplicationData(aboutData);

    // This is a very very very ugly hack that attempts to solve the following problem:
    // D-Bus service activated applications inherit the environment of dbus-daemon.
    // Normally, in KDE, startkde sets these environment variables. However, the session's
    // dbus-daemon is started before this happens, which means that dbus-daemon does NOT
    // have these variables in its environment and therefore all service-activated UIs
    // think that they are not running in KDE. This causes Qt not to load the KDE platform
    // plugin, which leaves the UI in a sorry state, using a completely wrong theme,
    // wrong colors, etc...
    // See also:
    // - https://bugs.kde.org/show_bug.cgi?id=269861
    // - https://bugs.kde.org/show_bug.cgi?id=267770
    // - https://git.reviewboard.kde.org/r/102194/
    // Here we are just going to assume that kde-telepathy is always used in KDE and
    // not anywhere else. This is probably the best that we can do.
    setenv("KDE_FULL_SESSION", "true", 0);
    setenv("KDE_SESSION_VERSION", "5", 0);

    KTp::TelepathyHandlerApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon::fromTheme(QLatin1String("telepathy-kde")));

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
        qDebug() << "No handlers registered. Exiting";
        return 1;
    }

    return app.exec();
}
