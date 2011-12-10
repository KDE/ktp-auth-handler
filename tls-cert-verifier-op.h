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

#ifndef TLSCERTVERIFIEROP_H
#define TLSCERTVERIFIEROP_H

#include <TelepathyQt/Account>
#include <TelepathyQt/AuthenticationTLSCertificateInterface>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

// FIXME: Move this to tp-qt4 itself
#include "types.h"

class TlsCertVerifierOp : public Tp::PendingOperation
{
    Q_OBJECT

public:
    TlsCertVerifierOp(const Tp::AccountPtr &account,
            const Tp::ConnectionPtr &connection,
            const Tp::ChannelPtr &channel);
    ~TlsCertVerifierOp();

Q_SIGNALS:
    void ready(Tp::PendingOperation *self);

private Q_SLOTS:
    void gotProperties(Tp::PendingOperation *op);

private:
    Tp::AccountPtr m_account;
    Tp::ConnectionPtr m_connection;
    Tp::ChannelPtr m_channel;
    QString m_hostname;
    QStringList m_referenceIdentities;
    Tp::Client::AuthenticationTLSCertificateInterface *m_authTLSCertificateIface;
    QString m_certType;
    CertificateDataList m_certData;
};

#endif
