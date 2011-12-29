/*
 * Copyright (C) 2011 Daniele E. Domenichelli <daniele.domenichelli@gmail.com>
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

#include "x-messenger-oauth2-prompt.h"

#include <KUrl>
#include <KIcon>
#include <KToolInvocation>
#include <KDebug>
#include <KWebView>
#include <KWebPage>

#include <QtWebKit/QWebSettings>
#include <QtGui/QProgressBar>
#include <QtGui/QBoxLayout>
#include <QtGui/QLayout>
#include <QtNetwork/QNetworkReply>


const QLatin1String msnClientID("000000004C070A47");
const QLatin1String redirectUri("https://oauth.live.com/desktop");
const QLatin1String request("https://oauth.live.com/authorize?response_type=token&redirect_uri=%1&client_id=%2&scope=wl.messenger");
const QLatin1String tokenParameter("access_token=");

XMessengerOAuth2Prompt::XMessengerOAuth2Prompt(QWidget* parent) :
    KDialog(parent),
    m_webView(new KWebView()),
    m_ProgressBar(new QProgressBar()),
    m_loginPageLoaded(false)
{
    // TODO Use .ui file
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
//    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    m_ProgressBar->setRange(0, 100);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);
    layout->addWidget(m_ProgressBar);

    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setContentsMargins(0, 0, 0, 0);

    setMainWidget(widget);
    setWindowIcon(KIcon("telepathy-kde"));
    setButtons(Cancel);

    // connect progress bar
    connect(m_webView,
            SIGNAL(loadFinished(bool)),
            m_ProgressBar,
            SLOT(hide()));
    connect(m_webView,
            SIGNAL(loadStarted()),
            m_ProgressBar,
            SLOT(show()));
    connect(m_webView,
            SIGNAL(loadProgress(int)),
            m_ProgressBar,
            SLOT(setValue(int)));

    connect(m_webView,
            SIGNAL(urlChanged(QUrl)),
            SLOT(onUrlChanged(QUrl)));
    connect(m_webView,
            SIGNAL(loadFinished(bool)),
            SLOT(onLoadFinished(bool)));

    connect(m_webView,
            SIGNAL(linkClicked(QUrl)),
            SLOT(onLinkClicked(QUrl)));

    KWebPage *page = qobject_cast<KWebPage*>(m_webView->page());

    page->setLinkDelegationPolicy(QWebPage::DontDelegateLinks);
    page->setForwardUnsupportedContent(true);

    connect(page,
            SIGNAL(unsupportedContent(QNetworkReply*)),
            SLOT(onUnsupportedContent(QNetworkReply*)));

    // start loading the login URL
    KUrl url(QString(request).arg(redirectUri).arg(msnClientID));
    m_webView->load(url.url());
}

XMessengerOAuth2Prompt::~XMessengerOAuth2Prompt()
{
}

QByteArray XMessengerOAuth2Prompt::accessToken() const
{
    return m_accessToken;
}

QSize XMessengerOAuth2Prompt::sizeHint() const
{
    return QSize(500, 600);
}

void XMessengerOAuth2Prompt::onUrlChanged(const QUrl &url)
{
    kDebug() << url;
    if (url.toString().indexOf(redirectUri) != 0) {
        // This is not the url containing the token
        return;
    }
    extractToken(url);
}

void XMessengerOAuth2Prompt::onLinkClicked(const QUrl& url)
{
    kDebug() << url;
    KToolInvocation::invokeBrowser(url.toString());
}

void XMessengerOAuth2Prompt::onLoadFinished(bool ok)
{
    if (!m_loginPageLoaded) {
        m_loginPageLoaded = true;
        Q_EMIT loginPageLoaded(ok);
    }
}

void XMessengerOAuth2Prompt::onUnsupportedContent(QNetworkReply* reply)
{
    // With QtWebkit < 2.2, for some reason the request for the token is
    // unsupported, but we can extract the token from the url of the failing
    // request
    // FIXME: In the future this might want to remove this
    QUrl url =  reply->url();
    if (url.toString().indexOf(redirectUri) != 0) {
        // This is not the url containing the token
        return;
    }
    extractToken(url);
}

void XMessengerOAuth2Prompt::extractToken(const QUrl &url)
{
    QString accessToken;
    Q_FOREACH (const QString &token, QString(url.encodedFragment()).split('&')) {
        // Get the URL fragment part and iterate over the parameters of the request
        if (token.indexOf(tokenParameter) == 0) {
            // This is the token that we are looking for (we are not interested
            // in the "expires_in" part, etc.)
            accessToken = token;
            break;
        }
    }
    if (accessToken.isEmpty()) {
        // Could not find the token
        kWarning() << "Token not found";
        return;
    }

    accessToken = accessToken.split('=').at(1);    // Split by "access_token=..." and take latter part

    // Wocky will base64 encode, but token actually already is base64, so we
    // decode now and it will be re-encoded.
    m_accessToken = QByteArray::fromBase64(QByteArray::fromPercentEncoding(accessToken.toUtf8()));
    Q_EMIT accessTokenTaken(m_accessToken);
    accept();
}

#include "x-messenger-oauth2-prompt.moc"
