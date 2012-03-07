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

#include "x-telepathy-captcha-prompt.h"
#include "ui_x-telepathy-captcha-prompt.h"

#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/Captcha>
#include <TelepathyQt/PendingCaptchas>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ServerAuthenticationChannel>

#include <KIcon>
#include <KDebug>

XTelepathyCaptchaPrompt::XTelepathyCaptchaPrompt(const Tp::ChannelPtr &channel, QWidget *parent)
    : KDialog(parent),
      ui(new Ui::XTelepathyCaptchaPrompt)
{
    m_channel = Tp::ServerAuthenticationChannelPtr::qObjectCast(channel);
    ui->setupUi(mainWidget());

    setWindowIcon(KIcon(QLatin1String("telepathy-kde")));
    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::User1);
    setButtonText(KDialog::User1, i18n("Reload"));

    ui->accountIcon->setPixmap(KIcon(QLatin1String("dialog-password")).pixmap(60, 60));

    requestCaptcha();
    KDialog::show();
}

XTelepathyCaptchaPrompt::~XTelepathyCaptchaPrompt()
{
    delete ui;
}

void XTelepathyCaptchaPrompt::onRequestCaptchasFinished(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        qWarning() << "Captchas Finished error: " << operation->errorMessage() << operation->errorName();
        return;
    }

    Tp::PendingCaptchas *captchas = qobject_cast<Tp::PendingCaptchas*>(operation);
    Tp::Captcha captcha = captchas->captcha();

    if (!captcha.label().isEmpty())
        ui->title->setText(captcha.label());
    else
        if (captcha.type() == Tp::CaptchaAuthentication::OCRChallenge)
            ui->title->setText(i18n("Enter the text below"));

    QPixmap pixmap;
    pixmap.loadFromData(captcha.data());
    pixmap = pixmap.scaledToHeight(100, Qt::SmoothTransformation);
    ui->imageLabel->setPixmap(pixmap);

    m_currentCaptchaId = captcha.id();

    ui->captchaLineEdit->clear();

    // Enable the buttons.
}

void XTelepathyCaptchaPrompt::requestCaptcha()
{
    Tp::CaptchaAuthenticationPtr captchaAuth = m_channel->captchaAuthentication();
    connect(captchaAuth->requestCaptchas(), SIGNAL(finished(Tp::PendingOperation*)),
            this, SLOT(onRequestCaptchasFinished(Tp::PendingOperation*)));
}

void XTelepathyCaptchaPrompt::onAnswerFinished(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        qWarning() << "Answer error" << operation->errorMessage() << operation->errorName();
        // Reload the captcha since we got it wrong (probably).
        requestCaptcha();
        return;
    }

    // Not an error, so close the dialog/window.
    connect(m_channel->requestClose(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(deleteLater()));
    accept();
}

void XTelepathyCaptchaPrompt::onCancelFinished(Tp::PendingOperation *operation)
{
    connect(m_channel->requestClose(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(deleteLater()));
    reject();
}

void XTelepathyCaptchaPrompt::slotButtonClicked(int button)
{
    switch (button)
    {
    case KDialog::Ok:
        {
            QString answer = ui->captchaLineEdit->text();
            Tp::CaptchaAuthenticationPtr captchaAuth = m_channel.data()->captchaAuthentication();
            connect(captchaAuth->answer(m_currentCaptchaId, answer), SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onAnswerFinished(Tp::PendingOperation*)));
        }
        break;
    case KDialog::Cancel:
        {
            Tp::CaptchaAuthenticationPtr captchaAuth = m_channel.data()->captchaAuthentication();
            connect(captchaAuth->cancel(Tp::CaptchaCancelReasonUserCancelled), SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(onCancelFinished(Tp::PendingOperation*)));
        }
        break;
    case KDialog::User1:
        {
            requestCaptcha();
        }
        break;
    }
}

#include "x-telepathy-captcha-prompt.moc"
