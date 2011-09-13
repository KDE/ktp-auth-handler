#include "password-prompt.h"
#include "ui_password-prompt.h"

#include <KIcon>
#include <KDebug>

PasswordPrompt::PasswordPrompt(const Tp::AccountPtr &account, QWidget *parent) :
    KDialog(parent),
    ui(new Ui::PasswordPrompt)
{
    ui->setupUi(mainWidget());

//    setWindowTitle(ki18n("Password Required"));
    setWindowIcon(KIcon("telepathy-kde"));

    ui->accountName->setText(account->displayName());
    ui->accountIcon->setPixmap(KIcon("dialog-password").pixmap(60,60));
    ui->title->setPixmap(KIcon(account->iconName()).pixmap(22,22));

    //dialog-password
}

PasswordPrompt::~PasswordPrompt()
{
    delete ui;
    kDebug() << "I've been deleted";
}

QString PasswordPrompt::password() const
{
    return ui->passwordLineEdit->text();
}

bool PasswordPrompt::savePassword() const
{
    return ui->savePassword->isChecked();
}
