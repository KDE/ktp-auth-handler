/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Library General Public                      *
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


#ifndef X_TELEPATHY_SSO_GOOGLE_OPERATION_H
#define X_TELEPATHY_SSO_GOOGLE_OPERATION_H

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Account>

class KJob;

class XTelepathySSOGoogleOperation : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(XTelepathySSOGoogleOperation)

public:
    explicit XTelepathySSOGoogleOperation(
            const Tp::AccountPtr &account,
            int accountStorageId,
            Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface);

private Q_SLOTS:
    void onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details);
    void gotCredentials(KJob *kjob);

private:
    Tp::AccountPtr m_account;
    Tp::Client::ChannelInterfaceSASLAuthenticationInterface *m_saslIface;

    int m_accountStorageId;;
    QByteArray m_challengeData;

    friend class SaslAuthOp;
};

#endif //X_TELEPATHY_SSO_GOOGLE_OPERATION_H
