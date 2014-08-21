/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  David Edmundson <kde@davidedmundson.co.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "x-telepathy-sso-facebook-operation.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/ChannelInterfaceSASLAuthenticationInterface>
#include <TelepathyQt/AccountInterfaceStorageInterface>

#include <TelepathyQt/PendingVariantMap>

#include <Accounts/Account>

#include <KUrl>
#include <QDebug>

#include "getcredentialsjob.h"

XTelepathySSOFacebookOperation::XTelepathySSOFacebookOperation(const Tp::AccountPtr& account, int accountStorageId, Tp::Client::ChannelInterfaceSASLAuthenticationInterface* saslIface) :
    PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface),
    m_accountStorageId(accountStorageId)
{
    Q_ASSERT(m_accountStorageId);
    connect(m_saslIface, SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)), SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
    connect(m_saslIface, SIGNAL(NewChallenge(QByteArray)), SLOT(onNewChallenge(QByteArray)));
}


void XTelepathySSOFacebookOperation::onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details)
{
    qDebug() << "New status is: " << status;
    qDebug() << "Details: " << details;
    qDebug() << "Reason: " << reason;

    switch (status){
    case Tp::SASLStatusNotStarted:
    {
        qDebug() << "starting credentials job";
        m_saslIface->StartMechanism(QLatin1String("X-FACEBOOK-PLATFORM"));
        break;
    }
    case Tp::SASLStatusServerSucceeded:
        qDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
        break;
    }
}

void XTelepathySSOFacebookOperation::onNewChallenge(const QByteArray& challengeData)
{
    qDebug() << "New Challenge" << challengeData;

    m_challengeData = challengeData;

    qDebug() << "GetCredentialsJob on account: " << m_accountStorageId;
    GetCredentialsJob *job = new GetCredentialsJob(m_accountStorageId, this);
    connect(job, SIGNAL(finished(KJob*)), SLOT(gotCredentials(KJob *)));
    job->start();
}

void XTelepathySSOFacebookOperation::gotCredentials(KJob *kJob)
{
    KUrl fbRequestUrl;
    KUrl fbResponseUrl;
    qDebug();

    fbRequestUrl.setQuery(m_challengeData);
    qDebug() << fbRequestQuery.queryItemValue("version");

    GetCredentialsJob *job = qobject_cast< GetCredentialsJob* >(kJob);
    QVariantMap credentialsData = job->credentialsData();
    fbResponseUrl.addQueryItem("method", fbRequestUrl.queryItemValue("method"));
    fbResponseUrl.addQueryItem("nonce", fbRequestUrl.queryItemValue("nonce"));
    fbResponseUrl.addQueryItem("access_token", credentialsData["AccessToken"].toString());
    fbResponseUrl.addQueryItem("api_key", credentialsData["ClientId"].toString());
    fbResponseUrl.addQueryItem("call_id", "0");
    fbResponseUrl.addQueryItem("v", "1.0");

    //.mid(1) trims leading '?' char
    m_saslIface->Respond(fbResponseUrl.query().mid(1).toUtf8());
    qDebug() << fbResponseQuery.query().mid(1);
}
