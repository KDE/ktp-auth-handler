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


#ifndef X_TELEPATHY_SSO_OPERATION_H
#define X_TELEPATHY_SSO_OPERATION_H

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Account>

class KJob;

class XTelepathySSOOperation : public Tp::PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(XTelepathySSOOperation)

public:
    explicit XTelepathySSOOperation(
            const Tp::AccountPtr &account,
            int accountStorageId,
            Tp::Client::ChannelInterfaceSASLAuthenticationInterface *saslIface);

private Q_SLOTS:
    void onSASLStatusChanged(uint status, const QString &reason, const QVariantMap &details);
    void onNewChallenge(const QByteArray &challengeData);
    void gotCredentials(KJob *kjob);

private:
    Tp::AccountPtr m_account;
    Tp::Client::ChannelInterfaceSASLAuthenticationInterface *m_saslIface;

    int m_accountStorageId;
    QByteArray m_challengeData;

    friend class SaslAuthOp;
};

#endif // X_TELEPATHY_SSO_OPERATION_H
