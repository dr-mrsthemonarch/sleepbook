#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    
private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onCancelClicked();
    
private:
    void setupUI();
    void setupLoginTab();
    void setupRegisterTab();
    
    QTabWidget* m_tabWidget;
    
    // Login tab
    QWidget* m_loginWidget;
    QLineEdit* m_loginUsername;
    QLineEdit* m_loginPassword;
    QPushButton* m_loginButton;
    QLabel* m_loginMessage;
    
    // Register tab
    QWidget* m_registerWidget;
    QLineEdit* m_registerUsername;
    QLineEdit* m_registerPassword;
    QLineEdit* m_registerPasswordConfirm;
    QLineEdit* m_registerDisplayName;
    QPushButton* m_registerButton;
    QLabel* m_registerMessage;
};

#endif // LOGINDIALOG_H