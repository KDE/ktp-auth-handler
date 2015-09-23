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
    //Check if the account has any StorageIdentifier, in which case we will
    //prioritize those mechanism related with KDE Accounts integration
    QScopedPointer<Tp::Client::AccountInterfaceStorageInterface> accountStorageInterface(
        new Tp::Client::AccountInterfaceStorageInterface(m_account->busName(), m_account->objectPath()));

    Tp::PendingVariantMap *pendingMap = accountStorageInterface->requestAllProperties();
    connect(pendingMap, SIGNAL(finished(Tp::PendingOperation*)), SLOT(onGetAccountStorageFetched(Tp::PendingOperation*)));
}

SaslAuthOp::~SaslAuthOp()
{
}

void SaslAuthOp::gotProperties(Tp::PendingOperation *op)
{
    if (m_mechanisms.isEmpty()) {
        if (op->isError()) {
            qWarning() << "Unable to retrieve available SASL mechanisms";
            m_channel->requestClose();
            setFinishedWithError(op->errorName(), op->errorMessage());
            return;
        }

        Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
        m_properties = qdbus_cast<QVariantMap>(pvm->result());
        m_mechanisms = qdbus_cast<QStringList>(m_properties.value(QLatin1String("AvailableMechanisms")));
        qDebug() << m_mechanisms;
    }

    uint status = qdbus_cast<uint>(m_properties.value(QLatin1String("SASLStatus")));
    QString error = qdbus_cast<QString>(m_properties.value(QLatin1String("SASLError")));
    QVariantMap errorDetails = qdbus_cast<QVariantMap>(m_properties.value(QLatin1String("SASLErrorDetails")));

    if (m_mechanisms.contains(QLatin1String("X-OAUTH2"))) {
        qDebug() << "Starting X-OAuth2 auth";
        m_mechanisms.removeAll(QStringLiteral("X-OAUTH2"));
        XTelepathySSOGoogleOperation *authop = new XTelepathySSOGoogleOperation(m_account, m_accountStorageId, m_saslIface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else if (m_mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        qDebug() << "Starting Password auth";
        m_mechanisms.removeAll(QStringLiteral("X-TELEPATHY-PASSWORD"));
        Q_EMIT ready(this);
        XTelepathyPasswordAuthOperation *authop = new XTelepathyPasswordAuthOperation(m_account, m_accountStorageId, m_saslIface, qdbus_cast<bool>(m_properties.value(QLatin1String("CanTryAgain"))));
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else {
        qWarning() << "X-TELEPATHY-PASSWORD, X-OAUTH2 are the only supported SASL mechanism and are not available:" << m_mechanisms;
        m_channel->requestClose();
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD, X-OAUTH2 are the only supported SASL mechanism and are not available:"));
        return;
    }
}

void SaslAuthOp::onAuthOperationFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        if (!m_mechanisms.isEmpty()) {
            // if we have other mechanisms left, try again with different one
            gotProperties(0);
        } else {
            setFinishedWithError(op->errorName(), op->errorMessage());
            m_channel->requestClose();
        }
    } else {
        setFinished();
        m_channel->requestClose();
    }
}

void SaslAuthOp::setReady()
{
    connect(m_saslIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}

void SaslAuthOp::onGetAccountStorageFetched(Tp::PendingOperation* op)
{
    Tp::PendingVariantMap *pendingMap = qobject_cast<Tp::PendingVariantMap*>(op);

    m_accountStorageId = pendingMap->result()["StorageIdentifier"].value<QDBusVariant>().variant().toInt();
    qDebug() << m_accountStorageId;

    setReady();
}
