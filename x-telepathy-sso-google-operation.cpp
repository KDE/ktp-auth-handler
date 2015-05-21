/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2 of the License, or (at your option) any later version.                 *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Library General Public License for more details.                                 *
 *                                                                                   *
 *  You should have received a copy of the GNU Library General Public License        *
 *  along with this library; see the file COPYING.LIB.  If not, write to             *
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,             *
 *  Boston, MA 02110-1301, USA.                                                      *
 *************************************************************************************/

#include "x-telepathy-sso-google-operation.h"

#include <KAccounts/getcredentialsjob.h>
#include <QDebug>

#include <KSharedConfig>
#include <KConfigGroup>

XTelepathySSOGoogleOperation::XTelepathySSOGoogleOperation(const Tp::AccountPtr &account, int accountStorageId, Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface)
    : PendingOperation(account)
    , m_saslIface(saslIface)
    , m_accountStorageId(accountStorageId)
{
    connect(m_saslIface, SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)), SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
}

void XTelepathySSOGoogleOperation::onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details)
{
    switch (status){
    case Tp::SASLStatusNotStarted:
    {
        qDebug() << "Status Not started";
        GetCredentialsJob *job = new GetCredentialsJob(m_accountStorageId, QStringLiteral("oauth2"), QStringLiteral("web_server"), this);
        connect(job, SIGNAL(finished(KJob*)), SLOT(gotCredentials(KJob*)));
        job->start();
        break;
    }
    case Tp::SASLStatusServerSucceeded:
        qDebug() << "Status Server Succeeded";
        m_saslIface->AcceptSASL();
        break;

    case Tp::SASLStatusSucceeded:
        qDebug() << "Authentication succeeded";
        setFinished();
        break;
    }
}

void XTelepathySSOGoogleOperation::gotCredentials(KJob *kjob)
{
    GetCredentialsJob *job = qobject_cast< GetCredentialsJob* >(kjob);
    QVariantMap credentialsData = job->credentialsData();

    QByteArray data;
    data.append("\0", 1);
    data.append(credentialsData["AccountUsername"].toByteArray());
    data.append("\0", 1);
    data.append(credentialsData["AccessToken"].toByteArray());

    qDebug() << "Received Google credentials, starting auth mechanism...";

    m_saslIface->StartMechanismWithData(QLatin1String("X-OAUTH2"), data);
}
