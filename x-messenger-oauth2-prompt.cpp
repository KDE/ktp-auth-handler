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

    KWebPage *page = reinterpret_cast<KWebPage*>(m_webView->page());

    page->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    page->setForwardUnsupportedContent(true);

    connect(page,
            SIGNAL(unsupportedContent(QNetworkReply*)),
            SLOT(onUnsupportedContent(QNetworkReply*)));

    // start loading the login URL
    KUrl url(QString(QLatin1String("https://oauth.live.com/authorize?response_type=token&redirect_uri=%1&client_id=%2&scope=wl.messenger")).arg(redirectUri).arg(msnClientID));
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
    // FIXME: This method is left for debugging purpose
    kDebug() << url;
    kDebug() << m_webView->url();
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
    // For some reason the request for the token is unsupported, but we
    // Can extract the token from the url of the failing request
    // FIXME: In the future this might need to be fixed
    QUrl url =  reply->url();
    if (url.toString().indexOf(redirectUri) != 0) {
        // This is not the url containing the token
        return;
    }
    QString accessToken = url.encodedFragment();   // Get the URL fragment part
    accessToken = accessToken.split("&").first();  // Remove the "expires_in" part.
    accessToken = accessToken.split("=").at(1);    // Split by "access_token=..." and take latter part

    // Wocky will base64 encode, but token actually already is base64, so we
    // decode now and it will be re-encoded.
    m_accessToken = QByteArray::fromBase64(QByteArray::fromPercentEncoding(accessToken.toUtf8()));
    kDebug() << "size = " << m_accessToken.size();
    char* data = QByteArray::fromBase64(QByteArray::fromPercentEncoding(accessToken.toUtf8())).data();
    for (int i = 0; i < m_accessToken.size(); i++, data++) {
     kDebug() << i << "[" << *data << "]" << endl;
    }
    Q_EMIT accessTokenTaken(m_accessToken);
    accept();
}

#include "x-messenger-oauth2-prompt.moc"
