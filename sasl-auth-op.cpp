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
#include "x-messenger-oauth2-auth-operation.h"

#ifdef HAVE_SSO
    #include "x-telepathy-sso-facebook-operation.h"
    #include "x-telepathy-sso-google-operation.h"
#endif

#include <QtCore/QScopedPointer>

#include <TelepathyQt/PendingVariantMap>

#include <KDebug>
#include <KLocalizedString>

#include <KTp/wallet-interface.h>
#include <KTp/pending-wallet.h>

SaslAuthOp::SaslAuthOp(const Tp::AccountPtr &account,
        const Tp::ChannelPtr &channel)
    : Tp::PendingOperation(channel),
      m_walletInterface(0),
      m_account(account),
      m_channel(channel),
      m_saslIface(channel->interface<Tp::Client::ChannelInterfaceSASLAuthentication1Interface>())
{
#ifdef HAVE_SSO
    m_accountStorageId = 0;
#endif

    //FIXME: We should open the wallet only when required (not required for X-FACEBOOK-PLATFORM)
    connect(KTp::WalletInterface::openWallet(), SIGNAL(finished(Tp::PendingOperation*)), SLOT(onOpenWalletOperationFinished(Tp::PendingOperation*)));
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
    QStringList mechanisms = qdbus_cast<QStringList>(props.value(QLatin1String("AvailableMechanisms")));
    kDebug() << mechanisms;

    uint status = qdbus_cast<uint>(props.value(QLatin1String("SASLStatus")));
    QString error = qdbus_cast<QString>(props.value(QLatin1String("SASLError")));
    QVariantMap errorDetails = qdbus_cast<QVariantMap>(props.value(QLatin1String("SASLErrorDetails")));

#ifdef HAVE_SSO
    if (mechanisms.contains(QLatin1String("X-FACEBOOK-PLATFORM")) && m_accountStorageId) {
        XTelepathySSOFacebookOperation *authop = new XTelepathySSOFacebookOperation(m_account, m_accountStorageId, m_saslIface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    }
    else if(mechanisms.contains(QLatin1String("X-OAUTH2")) && m_accountStorageId) {
        XTelepathySSOGoogleOperation *authop = new XTelepathySSOGoogleOperation(m_account, m_accountStorageId, m_saslIface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else //if...
#endif
    if (mechanisms.contains(QLatin1String("X-TELEPATHY-PASSWORD"))) {
        Q_EMIT ready(this);
        XTelepathyPasswordAuthOperation *authop = new XTelepathyPasswordAuthOperation(m_account, m_saslIface, m_walletInterface, qdbus_cast<bool>(props.value(QLatin1String("CanTryAgain"))));
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else if (mechanisms.contains(QLatin1String("X-MESSENGER-OAUTH2"))) {
        Q_EMIT ready(this);
        XMessengerOAuth2AuthOperation *authop = new XMessengerOAuth2AuthOperation(m_account, m_saslIface, m_walletInterface);
        connect(authop,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onAuthOperationFinished(Tp::PendingOperation*)));

        authop->onSASLStatusChanged(status, error, errorDetails);
    } else {
        kWarning() << "X-TELEPATHY-PASSWORD, X-MESSENGER-OAUTH2, X-OAUTH2, X-FACEBOOK_PLATFORM are the only supported SASL mechanism and are not available:" << mechanisms;
        m_channel->requestClose();
        setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("X-TELEPATHY-PASSWORD, X-MESSENGER-OAUTH2, X-OAUTH2, X-FACEBOOK_PLATFORM are the only supported SASL mechanism and are not available:"));
        return;
    }
}

void SaslAuthOp::onOpenWalletOperationFinished(Tp::PendingOperation *op)
{
    KTp::PendingWallet *walletOp = qobject_cast<KTp::PendingWallet*>(op);
    Q_ASSERT(walletOp);

    m_walletInterface = walletOp->walletInterface();

#ifdef HAVE_SSO
    fetchAccountStorage();
#else
    setReady();
#endif
}

void SaslAuthOp::onAuthOperationFinished(Tp::PendingOperation *op)
{
    m_channel->requestClose();
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
    } else {
        setFinished();
    }
}

void SaslAuthOp::setReady()
{
    connect(m_saslIface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotProperties(Tp::PendingOperation*)));
}

#ifdef HAVE_SSO
void SaslAuthOp::onGetAccountStorageFetched(Tp::PendingOperation* op)
{
    kDebug();
    Tp::PendingVariantMap *pendingMap = qobject_cast<Tp::PendingVariantMap*>(op);

    m_accountStorageId = pendingMap->result()["StorageIdentifier"].value<QDBusVariant>().variant().toInt();
    kDebug() << m_accountStorageId;

    setReady();
}

void SaslAuthOp::fetchAccountStorage()
{
    //Check if the account has any StorageIdentifier, in which case we will
    //prioritize those mechanism related with KDE Accounts integration
    QScopedPointer<Tp::Client::AccountInterfaceStorageInterface> accountStorageInterface(
        new Tp::Client::AccountInterfaceStorageInterface(m_account->busName(), m_account->objectPath()));

    Tp::PendingVariantMap *pendingMap = accountStorageInterface->requestAllProperties();
    connect(pendingMap, SIGNAL(finished(Tp::PendingOperation*)), SLOT(onGetAccountStorageFetched(Tp::PendingOperation*)));
}
#endif

#include "sasl-auth-op.moc"
