#ifndef PASSWORDPROMPT_H
#define PASSWORDPROMPT_H

#include <KDialog>

#include <TelepathyQt4/Account>

namespace Ui {
    class PasswordPrompt;
}

class PasswordPrompt : public KDialog
{
    Q_OBJECT

public:
    explicit PasswordPrompt(const Tp::AccountPtr &account, QWidget *parent = 0);
    ~PasswordPrompt();

    void setDefaultPassword() const;

    QString password() const;
    bool savePassword() const;

private:
    Ui::PasswordPrompt *ui;
};

#endif // PASSWORDPROMPT_H
