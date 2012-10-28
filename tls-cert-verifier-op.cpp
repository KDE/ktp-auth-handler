/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
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

#include "tls-cert-verifier-op.h"

#include <TelepathyQt/PendingVariantMap>

#include <KMessageBox>
#include <KLocalizedString>
#include <KDebug>

#include <QSslCertificate>

TlsCertVerifierOp::TlsCertVerifierOp(const Tp::AccountPtr &account,
        const Tp::ConnectionPtr &connection,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_account(account),
      m_connection(connection),
      m_channel(channel)
{
    QDBusObjectPath certificatePath = qdbus_cast<QDBusObjectPath>(channel->immutableProperties().value(
                TP_QT_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION + QLatin1String(".ServerCertificate")));
    m_hostname = qdbus_cast<QString>(channel->immutableProperties().value(
                TP_QT_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION + QLatin1String(".Hostname")));
    m_referenceIdentities = qdbus_cast<QStringList>(channel->immutableProperties().value(
                TP_QT_IFACE_CHANNEL_TYPE_SERVER_TLS_CONNECTION + QLatin1String(".ReferenceIdentities")));

    m_authTLSCertificateIface = new Tp::Client::AuthenticationTLSCertificateInterface(
            channel->dbusConnection(), channel->busName(), certificatePath.path());
    connect(m_authTLSCertificateIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}

TlsCertVerifierOp::~TlsCertVerifierOp()
{
}

void TlsCertVerifierOp::gotProperties(Tp::PendingOperation *op)
{
    if (op->isError()) {
        kWarning() << "Unable to retrieve properties from AuthenticationTLSCertificate object at" <<
            m_authTLSCertificateIface->path();
        m_channel->requestClose();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    // everything ok, we can return from handleChannels now
    Q_EMIT ready(this);

    Tp::PendingVariantMap *pvm = qobject_cast<Tp::PendingVariantMap*>(op);
    QVariantMap props = qdbus_cast<QVariantMap>(pvm->result());
    m_certType = qdbus_cast<QString>(props.value(QLatin1String("CertificateType")));
    m_certData = qdbus_cast<CertificateDataList>(props.value(QLatin1String("CertificateChainData")));

    if(m_certType.compare(QLatin1String("x509"), Qt::CaseInsensitive)) {
        kWarning() << "This is not an x509 certificate";
    }

    Q_FOREACH (const QByteArray &data, m_certData) {
        // FIXME How to chech if it is QSsl::Pem or QSsl::Der? QSsl::Der works for kdetalk
        QList<QSslCertificate> certs = QSslCertificate::fromData(data, QSsl::Der);
        Q_FOREACH (const QSslCertificate &cert, certs) {
            kDebug() << cert;
        }
    }

    //TODO Show a nice dialog
    if (KMessageBox::questionYesNo(0,
                                   i18n("<b>Accept this certificate?</b><br />%1").arg(QString::fromLatin1(m_certData.first().toHex())),
                                   i18n("Untrusted certificate")) == KMessageBox::Yes) {
        // TODO Remember value
        m_authTLSCertificateIface->Accept().waitForFinished();
        setFinished();
    } else {
        Tp::TLSCertificateRejectionList rejections;
        // TODO Add reason
        m_authTLSCertificateIface->Reject(rejections);
        m_channel->requestClose();
        setFinishedWithError(QLatin1String("Cert.Untrusted"),
                             QLatin1String("Certificate rejected by the user"));
    }
}

#include "tls-cert-verifier-op.moc"
