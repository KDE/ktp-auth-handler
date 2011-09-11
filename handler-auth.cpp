/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) David Edmundson <kde@davidedmundson.co.uk>
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

#include "handler-auth.h"

#include <TelepathyQt4/PendingVariant>

#include <KDebug>

#include "password-prompt.h"

HandlerAuth::HandlerAuth(const Tp::AccountPtr &account,
    const Tp::ConnectionPtr &connection,
    const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_account(account),
      m_connection(connection),
      m_channel(new SaslChannel(channel))
{
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                  0,
                                  KWallet::Wallet::Asynchronous);


    Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface =
        channel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>();
    connect(saslIface,
            SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)),
            SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
    connect(saslIface->requestPropertyAvailableMechanisms(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotAvailableSASLMechanisms(Tp::PendingOperation*)));
}

HandlerAuth::~HandlerAuth()
{
}

void HandlerAuth::gotAvailableSASLMechanisms(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to retrieve available SASL mechanisms";
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    Tp::PendingVariant *pv = qobject_cast<Tp::PendingVariant*>(op);
    QStringList mechanisms = qdbus_cast<QStringList>(pv->result());
    if (!mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        setFinishedWithError(TP_QT4_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD is the only supported SASL mechanisms"));
        return;
    }

    promptUser(true);
}

void HandlerAuth::onSASLStatusChanged(uint status, const QString &reason,
        const QVariantMap &details)
{
    if (status == Tp::SASLStatusServerSucceeded) {

        m_channel->acceptSasl();
        kDebug() << "Authentication handshake";
    } else if (status == Tp::SASLStatusSucceeded) {
        kDebug() << "Authentication succeeded";
        setFinished();
    } else if (status == Tp::SASLStatusInProgress) {
        kDebug() << "Authenticating...";
    } else if (status == Tp::SASLStatusServerFailed) {
        kDebug() << "Error authenticating - reason:" << reason << "- details:" << details;

        //FIXME add - if can retry.
        promptUser(false);
    }
}

void HandlerAuth::promptUser(bool isFirstRun)
{
    //if we have a password

    kDebug() << "trying to load from wallet";
    if (m_wallet->hasFolder("telepathy") && isFirstRun) {
        m_wallet->setFolder("telepathy");

        QString password;
        kDebug() << "has telepathy folder";

        if (m_wallet->hasEntry(m_account->uniqueIdentifier())) {
            kDebug() << "has entry";

            int returnValue = m_wallet->readPassword(m_account->uniqueIdentifier(), password);
            if (returnValue == 0) {
                kDebug() << "using saved password";
                m_channel->startMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"),
                                                  password.toUtf8());
                return;
            }
            else {
                kDebug() << "Error reading wallet entry";
            }
        }
    }

    //otherwise prompt the user

    PasswordPrompt dialog(m_account);

    if (dialog.exec() == QDialog::Rejected) {
        qDebug() << "Authentication canceled";
        m_channel->close();
        return;
    }

    qDebug() << "Starting authentication...";

    m_channel->startMechanismWithData(QLatin1String("X-TELEPATHY-PASSWORD"),
            dialog.password().toUtf8());

    //save password in kwallet...
    if (dialog.savePassword()) {
        kDebug() << "Saving password";
        if (!m_wallet->hasFolder("telepathy")) {
            m_wallet->createFolder("telepathy");
        }
        m_wallet->setFolder("telepathy");
        m_wallet->writePassword(m_account->uniqueIdentifier(), dialog.password());
    }
}




#include "handler-auth.moc"
