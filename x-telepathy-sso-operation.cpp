/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  David Edmundson <D.Edmundson@lboro.ac.uk>

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

#include <TelepathyQt/Account>
#include <TelepathyQt/ChannelInterfaceSASLAuthenticationInterface>

#include <Accounts/Account>

#include <KDebug>
#include <KUrl>

#include "x-telepathy-sso-operation.h"
#include "getcredentialsjob.h"

XTelepathySSOOperation::XTelepathySSOOperation(const Tp::AccountPtr& account, Tp::Client::ChannelInterfaceSASLAuthenticationInterface* saslIface) :
    PendingOperation(account),
    m_account(account),
    m_saslIface(saslIface)
{
    connect(m_saslIface, SIGNAL(SASLStatusChanged(uint,QString,QVariantMap)), SLOT(onSASLStatusChanged(uint,QString,QVariantMap)));
    connect(m_saslIface, SIGNAL(NewChallenge(QByteArray)), SLOT(onNewChallenge(QByteArray)));
}


void XTelepathySSOOperation::onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details)
{
    qDebug() << "new status is " << status;
    qDebug() << "details" << details;
    switch (status){
    case Tp::SASLStatusNotStarted:
        kDebug() << "starting credentials job";     
        m_saslIface->StartMechanism(QLatin1String("X-FACEBOOK-PLATFORM"));
    case Tp::SASLStatusServerSucceeded:
        kDebug() << "Authentication handshake";
        m_saslIface->AcceptSASL();
        break;
    }
}

void XTelepathySSOOperation::onNewChallenge(const QByteArray& array)
{
    kDebug() << "new challenge" << array;  
    kDebug() << "responding again";
    
    GetCredentialsJob *job = new GetCredentialsJob(11, this);
    kDebug() << "made new credentials job..starting";
    job->exec();
    
    KUrl fbRequestUrl;
    KUrl fbResponseUrl;
    
    fbRequestUrl.setQuery(array);
    kDebug() << fbRequestUrl.queryItemValue("version");

    fbResponseUrl.addQueryItem("method", fbRequestUrl.queryItemValue("method"));
    fbResponseUrl.addQueryItem("nonce", fbRequestUrl.queryItemValue("nonce"));
    fbResponseUrl.addQueryItem("access_token", job->credentialsData()["AccessToken"].toString());
    fbResponseUrl.addQueryItem("api_key", job->credentialsData()["ClientId"].toString());
    fbResponseUrl.addQueryItem("call_id", "0");
    fbResponseUrl.addQueryItem("v", "1.0");
    
    //.mid(1) trims leading '?' char
    kDebug() << fbResponseUrl.query().mid(1);
    m_saslIface->Respond(fbResponseUrl.query().mid(1).toUtf8());
}



