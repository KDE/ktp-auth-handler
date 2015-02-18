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

#include "sasl-auth-op.h"

#include "x-telepathy-password-auth-operation.h"
#include "x-telepathy-sso-facebook-operation.h"
#include "x-telepathy-sso-google-operation.h"

#include <QtCore/QScopedPointer>

#include <TelepathyQt/PendingVariantMap>

#include <QDebug>
#include <KLocalizedString>

SaslAuthOp::SaslAuthOp(const Tp::AccountPtr &account,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_account(account),
      m_channel(channel),
      m_saslIface(channel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>())
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kaccounts-ktprc"));
    KConfigGroup ktpKaccountsGroup = config->group(QStringLiteral("ktp-kaccounts"));
    m_kaccountsId = ktpKaccountsGroup.readEntry(account->objectPath()).toUInt();

    setReady();
}

SaslAuthOp::~SaslAuthOp()
{
}

void SaslAuthOp::gotProperties(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to retrieve available SASL mechanisms";
        m_channel->requestClose();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
    QVariantMap props = qdbus_cast<QVariantMap>(pvm->result());
    QStringList mechanisms = qdbus_cast<QStringList>(props.value(QLatin1String("AvailableMechanisms")));
    qDebug() << mechanisms;

    uint status = qdbus_cast<uint>(props.value(QLatin1String("SASLStatus")));
    QString error = qdbus_cast<QString>(props.value(QLatin1String("SASLError")));
    QVariantMap errorDetails = qdbus_cast<QVariantMap>(props.value(QLatin1String("SASLErrorDetails")));

    if (mechanisms.contains(QLatin1String("X-FACEBOOK-PLATFORM")) && m_kaccountsId != 0) {
        qDebug() << "Starting Facebook OAuth auth";
        XTelepathySSOFacebookOperation *authop = new XTelepathySSOFacebookOperation(m_account, m_kaccountsId, m_saslIface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else if (mechanisms.contains(QLatin1String("X-OAUTH2")) && m_kaccountsId != 0) {
        qDebug() << "Starting X-OAuth2 auth";
        XTelepathySSOGoogleOperation *authop = new XTelepathySSOGoogleOperation(m_account, m_kaccountsId, m_saslIface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else if (mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        qDebug() << "Starting Password auth";
        Q_EMIT ready(this);
        XTelepathyPasswordAuthOperation *authop = new XTelepathyPasswordAuthOperation(m_account, m_saslIface, qdbus_cast<bool>(props.value(QLatin1String("CanTryAgain"))));
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else {
        qWarning() << "X-TELEPATHY-PASSWORD, X-OAUTH2, X-FACEBOOK_PLATFORM are the only supported SASL mechanism and are not available:" << mechanisms;
        m_channel->requestClose();
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD, X-OAUTH2, X-FACEBOOK_PLATFORM are the only supported SASL mechanism and are not available:"));
        return;
    }
}

void SaslAuthOp::onAuthOperationFinished(Tp::PendingOperation *op)
{
    m_channel->requestClose();
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        setFinished();
    }
}

void SaslAuthOp::setReady()
{
    connect(m_saslIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}
