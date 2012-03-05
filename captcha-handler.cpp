/*
 * Copyright (C) 2012 Jeremy Whiting <jpwhiting@kde.org>
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

#include "captcha-handler.h"

#include "x-telepathy-captcha-prompt.h"

#include <KTp/telepathy-handler-application.h>

#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ChannelClassSpec>

inline Tp::ChannelClassSpecList channelClassList()
{
    Tp::ChannelClassSpec spec = Tp::ChannelClassSpec(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION,
                                                     Tp::HandleTypeNone);
    spec.setProperty(TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION + QLatin1String(".AuthenticationMethod"),
                     TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION);

    return Tp::ChannelClassSpecList() << spec;
}

CaptchaHandler::CaptchaHandler()
  : AbstractClientHandler(channelClassList())
{
    /* Registering telepathy types */
    Tp::registerTypes();
}

CaptchaHandler::~CaptchaHandler()
{
}

bool CaptchaHandler::bypassApproval() const
{
    // Don't bypass approval of channels.
    return false;
}

void CaptchaHandler::handleChannels(const Tp::MethodInvocationContextPtr<> &context,
                                    const Tp::AccountPtr &account,
                                    const Tp::ConnectionPtr &connection,
                                    const QList<Tp::ChannelPtr> &channels,
                                    const QList<Tp::ChannelRequestPtr> &requestsSatisfied,
                                    const QDateTime &userActionTime,
                                    const HandlerInfo &handlerInfo)
{
    Q_UNUSED(account);
    Q_UNUSED(connection);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    Q_FOREACH (const Tp::ChannelPtr &channel, channels) {
        qDebug() << "Incoming channel: " << channel;

        QVariantMap properties = channel->immutableProperties();

        if (properties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] ==
               TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION) {

            qDebug() << "Channel is a captcha request. Handling: " << channel;
            KTp::TelepathyHandlerApplication::newJob();
            // FeatureCaptcha should be ready at this point thanks to our factory
            new XTelepathyCaptchaPrompt(channel);
        }
    }

    context->setFinished();
}
