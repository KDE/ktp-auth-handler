/*
 * Copyright (C) 2011, 2012 Daniele E. Domenichelli <daniele.domenichelli@gmail.com>
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

#include <QtGui/QProgressBar>
#include <QtGui/QBoxLayout>
#include <QtGui/QLayout>
#include <QtNetwork/QNetworkReply>

#include <QtWebKit/QWebSettings>
#include <QtWebKit/QWebFrame>

#include <qjson/parser.h>


const QLatin1String msnClientID("000000004C070A47");
const QLatin1String scopes("wl.messenger wl.offline_access");
const QLatin1String redirectUri("https://oauth.live.com/desktop");
const QLatin1String tokenUri("https://oauth.live.com/token");

const QLatin1String authorizeRequest("https://oauth.live.com/authorize?client_id=%1&scope=%2&response_type=code&redirect_uri=%3");
const QLatin1String tokenRequest("https://oauth.live.com/token?client_id=%1&redirect_uri=%2&code=%3&grant_type=authorization_code");

const QLatin1String codeParameter("code");
const QLatin1String accessTokenParameter("access_token");
const QLatin1String refreshTokenParameter("refresh_token");

XMessengerOAuth2Prompt::XMessengerOAuth2Prompt(QWidget* parent) :
    KDialog(parent),
    m_webView(new KWebView()),
    m_ProgressBar(new QProgressBar()),
    m_loginPageLoaded(false)
{
    // TODO Use .ui file
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    m_webView->setAcceptDrops(false);
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
    setWindowIcon(KIcon(QLatin1String("telepathy-kde")));
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
    KUrl url(QString(authorizeRequest).arg(msnClientID).arg(scopes).arg(redirectUri));
    m_webView->load(url.url());
}

XMessengerOAuth2Prompt::~XMessengerOAuth2Prompt()
{
}

QByteArray XMessengerOAuth2Prompt::accessToken() const
{
    return m_accessToken;
}

QByteArray XMessengerOAuth2Prompt::refreshToken() const
{
    return m_refreshToken;
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
    kDebug() << "This is the url we are waiting for";
    extractCode(url);
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
    if (m_webView->url().toString().indexOf(tokenUri) != 0) {
        // This is not the url containing the token
        return;
    }
    extractTokens(m_webView->page()->currentFrame()->toPlainText());

}

void XMessengerOAuth2Prompt::onUnsupportedContent(QNetworkReply* reply)
{
    // With QtWebkit < 2.2, for some reason the request for the token is
    // unsupported, but we can extract the token from the url of the failing
    // request
    // FIXME: In the future this might want to remove this
    QUrl url =  reply->url();
    kDebug() << url;
    if (url.toString().indexOf(redirectUri) != 0) {
        // This is not the url containing the token
        return;
    }
    extractCode(url);
}

void XMessengerOAuth2Prompt::extractCode(const QUrl &url)
{
    kDebug() << url;
    QString code = url.queryItemValue(codeParameter);

    if (code.isEmpty()) {
        // Could not find the access token
        kWarning() << "Code not found";
        return;
    }

    kDebug() << "Code found:" << code;

    // start loading the login URL
    KUrl tokenUrl(QString(tokenRequest).arg(msnClientID).arg(redirectUri).arg(code));
    m_webView->load(tokenUrl.url());
}


void XMessengerOAuth2Prompt::extractTokens(const QString &text)
{
    kDebug() << text;

    QJson::Parser parser;
    bool ok;

    QVariantMap result = parser.parse(text.toLatin1(), &ok).toMap();
    if (!ok) {
        kWarning() << "An error occured during parsing reply";
        return;
    }

    if (!result.contains(refreshTokenParameter)) {
        // Could not find the refresh token
        kWarning() << "Refresh token not found";
        return;
    }

    m_refreshToken = result[refreshTokenParameter].toByteArray();
    Q_EMIT refreshTokenTaken(m_refreshToken);

    if (!result.contains(accessTokenParameter)) {
        // Could not find the access token
        kWarning() << "Access token not found";
        return;
    }

    // Wocky will base64 encode, but token actually already is base64, so we
    // decode now and it will be re-encoded.
    m_accessToken = QByteArray::fromBase64(QByteArray::fromPercentEncoding(result[accessTokenParameter].toByteArray()));
    Q_EMIT accessTokenTaken(m_accessToken);

    accept();
}

#include "x-messenger-oauth2-prompt.moc"
