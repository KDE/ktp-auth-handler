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

#ifndef X_MESSENGER_OAUTH2_PROMPT_H
#define X_MESSENGER_OAUTH2_PROMPT_H

#include <KDialog>

class QProgressBar;
class KWebView;
class QNetworkReply;

class XMessengerOAuth2Prompt : public KDialog
{
    Q_OBJECT

public:
    explicit XMessengerOAuth2Prompt(QWidget *parent = 0);
    ~XMessengerOAuth2Prompt();

    QByteArray accessToken() const;

protected:
    virtual QSize sizeHint() const;

Q_SIGNALS:
    void loginPageLoaded(bool ok);
    void accessTokenTaken(const QByteArray &accessToken);

private Q_SLOTS:
    void onUrlChanged(const QUrl &url);
    void onLinkClicked(const QUrl &url);
    void onLoadFinished(bool);
    void onUnsupportedContent(QNetworkReply* reply);

private:
    void extractToken(const QUrl &url);

    KWebView *m_webView;
    QProgressBar *m_ProgressBar;
    bool m_loginPageLoaded;
    QByteArray m_accessToken;
};

#endif // X_MESSENGER_OAUTH2_PROMPT_H
