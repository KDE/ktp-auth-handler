/*
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 *   @author Andre Moreira Magalhaes <andre.magalhaes@collabora.co.uk>
 * Copyright (C) 2013 Dan Vrátil <dvratil@redhat.com>
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
#include <QSslCipher>

#include "kssl/ksslcertificatemanager.h"
#include "kssl/ksslinfodialog.h"

#include <QtCrypto>

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

    //compare returns 0 on match. We run this if cert type does not match x509 or "x509"
    //we also seem to need to check for "x509" and x509.
    if (m_certType.compare(QLatin1String("\"x509\""), Qt::CaseInsensitive) != 0 &&
        m_certType.compare(QLatin1String("x509"), Qt::CaseInsensitive) != 0) {
        Tp::TLSCertificateRejectionList rejections;
        m_authTLSCertificateIface->Reject(rejections);
        m_channel->requestClose();
        setFinishedWithError(QLatin1String("Cert.Unknown"),
                             QString::fromLatin1("Invalid certificate type %1").arg(m_certType));
        return;
    }

    // Initialize QCA module
    QCA::Initializer initializer;

    QCA::CertificateChain chain;
    Q_FOREACH (const QByteArray &data, m_certData) {
       chain << QCA::Certificate::fromDER(data);
    }

    if (verifyCertChain(chain)) {
        m_authTLSCertificateIface->Accept().waitForFinished();
        setFinished();
    } else {
        Tp::TLSCertificateRejectionList rejections;
        m_authTLSCertificateIface->Reject(rejections);
        m_channel->requestClose();
        setFinishedWithError(QLatin1String("Cert.Untrusted"),
                             QLatin1String("Certificate rejected by the user"));
    }
}

bool TlsCertVerifierOp::verifyCertChain(const QCA::CertificateChain& chain)
{
    const QList<QSslCertificate> primary = QSslCertificate::fromData(chain.primary().toDER(), QSsl::Der);
    KSslCertificateManager *const cm = KSslCertificateManager::self();
    KSslCertificateRule rule = cm->rule(primary.first(), m_hostname);

    // Find all errors then are not ignored by the rule
    QList<KSslError> errors;

    QCA::Validity validity = chain.validate(CACollection());
    if (validity != QCA::ValidityGood) {
        KSslError::Error error = validityToError(validity);
        if (!rule.ignoredErrors().contains(error)) {
            errors << KSslError(error);
        }
    }

    // If all errors are ignored, just accept
    if (errors.isEmpty()) {
        return true;
    }

    QString message = i18n("The server failed the authenticity check (%1).\n\n", m_hostname);
    Q_FOREACH(const KSslError &error, errors) {
        message.append(error.errorString());
        message.append(QLatin1Char('\n'));
    }

    int msgResult;
    do {
        msgResult = KMessageBox::warningYesNoCancel(0,
                               message,
                               i18n("Server Authentication"),
                               KGuiItem(i18n("&Details"), QLatin1String("dialog-information")), // yes
                               KGuiItem(i18n("Co&ntinue"), QLatin1String("arrow-right")), // no
                               KGuiItem(i18n("&Cancel"), QLatin1String("dialog-cancel")));
        if (msgResult == KMessageBox::Yes) {
            showSslDialog(chain, errors);
        } else if (msgResult == KMessageBox::Cancel) {
            return false; // reject
        }
        // Fall through on KMessageBox::No
    } while (msgResult == KMessageBox::Yes);

    // Save the user's choice to ignore the SSL errors.
    msgResult = KMessageBox::warningYesNo(
                            0,
                            i18n("Would you like to accept this "
                                 "certificate forever without "
                                 "being prompted?"),
                            i18n("Server Authentication"),
                            KGuiItem(i18n("&Forever"), QLatin1String("flag-green")),
                            KGuiItem(i18n("&Current Session only"), QLatin1String("chronometer")));
    QDateTime ruleExpiry = QDateTime::currentDateTime();
    if (msgResult == KMessageBox::Yes) {
        // Accept forever ("for a very long time")
        ruleExpiry = ruleExpiry.addYears(1000);
    } else {
        // Accept "for a short time", half an hour.
        ruleExpiry = ruleExpiry.addSecs(30*60);
    }

    rule.setExpiryDateTime(ruleExpiry);
    rule.setIgnoredErrors(errors);
    cm->setRule(rule);

    return true;
}

void TlsCertVerifierOp::showSslDialog(const QCA::CertificateChain &chain, const QList<KSslError> &errors) const
{
    QString errorStr;
    Q_FOREACH (const KSslError &error, errors) {
        errorStr += QString::number(static_cast<int>(error.error())) + QLatin1Char('\t');
        errorStr += QLatin1Char('\n');
    }
    errorStr.chop(1);

    // No way to tell whether QSsl::TlsV1 or QSsl::TlsV1Ssl3
    KSslCipher cipher = QSslCipher(QLatin1String("TLS"), QSsl::TlsV1);
    QString sslCipher = cipher.encryptionMethod() + QLatin1Char('\n');
    sslCipher += cipher.authenticationMethod() + QLatin1Char('\n');
    sslCipher += cipher.keyExchangeMethod() + QLatin1Char('\n');
    sslCipher += cipher.digestMethod();

    const QList<QSslCertificate> qchain = chainToList(chain);
    QPointer<KSslInfoDialog> dialog(new KSslInfoDialog(0));
    dialog->setSslInfo(qchain,
                    QString(), // we don't know the IP
                    m_hostname, // the URL
                    QLatin1String("TLS"),
                    sslCipher,
                    cipher.usedBits(),
                    cipher.supportedBits(),
                    KSslInfoDialog::errorsFromString(errorStr));

    dialog->exec();
    delete dialog;
}

KSslError::Error TlsCertVerifierOp::validityToError(QCA::Validity validity) const
{
    switch (validity) {
        case QCA::ValidityGood:
            return KSslError::NoError;
        case QCA::ErrorRejected:
            return KSslError::RejectedCertificate;
        case QCA::ErrorUntrusted:
            return KSslError::UntrustedCertificate;
        case QCA::ErrorSignatureFailed:
            return KSslError::CertificateSignatureFailed;
        case QCA::ErrorInvalidCA:
            return KSslError::InvalidCertificateAuthorityCertificate;
        case QCA::ErrorInvalidPurpose:
            return KSslError::InvalidCertificatePurpose;
        case QCA::ErrorSelfSigned:
            return KSslError::SelfSignedCertificate;
        case QCA::ErrorRevoked:
            return KSslError::RevokedCertificate;
        case QCA::ErrorPathLengthExceeded:
            return KSslError::PathLengthExceeded;
        case QCA::ErrorExpired: // fall-through
        case QCA::ErrorExpiredCA:
            return KSslError::ExpiredCertificate;
        case QCA::ErrorValidityUnknown:
            return KSslError::UnknownError;
    }

    return KSslError::UnknownError;
}

QCA::CertificateCollection TlsCertVerifierOp::CACollection() const
{
    QList<QSslCertificate> certs = KSslCertificateManager::self()->caCertificates();
    QCA::CertificateCollection collection;

    Q_FOREACH(const QSslCertificate &cert, certs) {
        collection.addCertificate(QCA::Certificate::fromDER(cert.toDer()));
    }

    return collection;
}

QList< QSslCertificate > TlsCertVerifierOp::chainToList(const QCA::CertificateChain& chain) const
{
    QList<QSslCertificate> certs;
    Q_FOREACH(const QCA::Certificate &cert, chain) {
        certs << QSslCertificate::fromData(cert.toDER(), QSsl::Der);
    }

    return certs;
}

#include "tls-cert-verifier-op.moc"
