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

#include <TelepathyQt/PendingVariantMap>

#include <KDebug>
#include <KLocalizedString>


SaslAuthOp::SaslAuthOp(const Tp::AccountPtr &account,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_account(account),
      m_channel(channel),
      m_saslIface(channel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>())
{
    connect(m_saslIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}

SaslAuthOp::~SaslAuthOp()
{
}

void SaslAuthOp::gotProperties(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kWarning() << "Unable to retrieve available SASL mechanisms";
        m_channel->requestClose();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
    QVariantMap props = qdbus_cast<QVariantMap>(pvm->result());
    QStringList mechanisms = qdbus_cast<QStringList>(props.value("AvailableMechanisms"));
    kDebug() << mechanisms;

    if (mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        // everything ok, we can return from handleChannels now
        emit ready(this);
        XTelepathyPasswordAuthOperation *authop = new XTelepathyPasswordAuthOperation(m_account, m_saslIface, qdbus_cast<bool>(props.value("CanTryAgain")));
        connect (authop,
                 SIGNAL(finished(Tp::PendingOperation*)),
                 SLOT(onAuthOperationFinished(Tp::PendingOperation*)));
        uint status = qdbus_cast<uint>(props.value("SASLStatus"));
        QString error = qdbus_cast<QString>(props.value("SASLError"));
        QVariantMap errorDetails = qdbus_cast<QVariantMap>(props.value("SASLErrorDetails"));
        authop->onSASLStatusChanged(status, error, errorDetails);
    } else {
        kWarning() << "X-TELEPATHY-PASSWORD is the only supported SASL mechanism and is not available:" << mechanisms;
        m_channel->requestClose();
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD is the only supported SASL mechanism and is not available"));
        return;
    }
}

void SaslAuthOp::onAuthOperationFinished(Tp::PendingOperation *op)
{
    m_channel->requestClose();
    if(op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        setFinished();
    }
}

#include "sasl-auth-op.moc"
