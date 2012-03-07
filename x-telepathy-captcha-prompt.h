/*
 * Copyright (C) 2012 Jeremy Whiting <jpwhiting>kde.org>
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

#ifndef CAPTCHAPROMPT_H
#define CAPTCHAPROMPT_H

#include <KDialog>

#include <TelepathyQt/Account>

namespace Ui
{
    class XTelepathyCaptchaPrompt;
}

class XTelepathyCaptchaPrompt : public KDialog
{
    Q_OBJECT

public:
    explicit XTelepathyCaptchaPrompt(const Tp::ChannelPtr &channel, QWidget *parent = 0);
    ~XTelepathyCaptchaPrompt();

protected Q_SLOTS:
    virtual void slotButtonClicked(int button);

private Q_SLOTS:
    void onRequestCaptchasFinished(Tp::PendingOperation *operation);
    void onAnswerFinished(Tp::PendingOperation *operation);
    void onCancelFinished(Tp::PendingOperation *operation);

private:
    void requestCaptcha();

    Ui::XTelepathyCaptchaPrompt *ui;
    Tp::ServerAuthenticationChannelPtr m_channel;
    unsigned int m_currentCaptchaId;
};

#endif // PASSWORDPROMPT_H
