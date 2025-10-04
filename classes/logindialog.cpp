#include "logindialog.h"
#include "usermanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
}

void LoginDialog::setupUI() {
    setWindowTitle("Sleep Tracker - Login");
    setModal(true);
    setMinimumWidth(400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("Welcome to Sleep Quality Tracker");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupLoginTab();
    setupRegisterTab();
    
    m_tabWidget->addTab(m_loginWidget, "Login");
    m_tabWidget->addTab(m_registerWidget, "Register");
}

void LoginDialog::setupLoginTab() {
    m_loginWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_loginWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);
    
    layout->addWidget(new QLabel("Username:"));
    m_loginUsername = new QLineEdit();
    m_loginUsername->setPlaceholderText("Enter your username");
    layout->addWidget(m_loginUsername);
    
    layout->addWidget(new QLabel("Password:"));
    m_loginPassword = new QLineEdit();
    m_loginPassword->setEchoMode(QLineEdit::Password);
    m_loginPassword->setPlaceholderText("Enter your password");
    layout->addWidget(m_loginPassword);
    
    m_loginMessage = new QLabel();
    m_loginMessage->setStyleSheet("color: red; font-size: 12px;");
    m_loginMessage->setWordWrap(true);
    layout->addWidget(m_loginMessage);
    
    layout->addSpacing(10);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("Login");
    m_loginButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px; font-weight: bold;");
    QPushButton* cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    layout->addStretch();
    
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::setupRegisterTab() {
    m_registerWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_registerWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);
    
    layout->addWidget(new QLabel("Username:"));
    m_registerUsername = new QLineEdit();
    m_registerUsername->setPlaceholderText("Choose a username");
    layout->addWidget(m_registerUsername);
    
    layout->addWidget(new QLabel("Display Name (optional):"));
    m_registerDisplayName = new QLineEdit();
    m_registerDisplayName->setPlaceholderText("Your name");
    layout->addWidget(m_registerDisplayName);
    
    layout->addWidget(new QLabel("Password:"));
    m_registerPassword = new QLineEdit();
    m_registerPassword->setEchoMode(QLineEdit::Password);
    m_registerPassword->setPlaceholderText("Choose a password");
    layout->addWidget(m_registerPassword);
    
    layout->addWidget(new QLabel("Confirm Password:"));
    m_registerPasswordConfirm = new QLineEdit();
    m_registerPasswordConfirm->setEchoMode(QLineEdit::Password);
    m_registerPasswordConfirm->setPlaceholderText("Re-enter password");
    layout->addWidget(m_registerPasswordConfirm);
    
    m_registerMessage = new QLabel();
    m_registerMessage->setStyleSheet("color: red; font-size: 12px;");
    m_registerMessage->setWordWrap(true);
    layout->addWidget(m_registerMessage);
    
    layout->addSpacing(10);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_registerButton = new QPushButton("Create Account");
    m_registerButton->setStyleSheet("background-color: #2196F3; color: white; padding: 8px; font-weight: bold;");
    QPushButton* cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(m_registerButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    layout->addStretch();
    
    connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    connect(m_registerPasswordConfirm, &QLineEdit::returnPressed, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::onLoginClicked() {
    QString username = m_loginUsername->text().trimmed();
    QString password = m_loginPassword->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        m_loginMessage->setText("Please enter both username and password.");
        return;
    }
    
    UserManager& manager = UserManager::instance();
    if (manager.authenticateUser(username, password)) {
        accept();
    } else {
        m_loginMessage->setText("Invalid username or password.");
        m_loginPassword->clear();
        m_loginPassword->setFocus();
    }
}

void LoginDialog::onRegisterClicked() {
    QString username = m_registerUsername->text().trimmed();
    QString displayName = m_registerDisplayName->text().trimmed();
    QString password = m_registerPassword->text();
    QString passwordConfirm = m_registerPasswordConfirm->text();
    
    // Validation
    if (username.isEmpty()) {
        m_registerMessage->setText("Please enter a username.");
        return;
    }
    
    if (username.length() < 3) {
        m_registerMessage->setText("Username must be at least 3 characters.");
        return;
    }
    
    if (password.isEmpty()) {
        m_registerMessage->setText("Please enter a password.");
        return;
    }
    
    if (password.length() < 4) {
        m_registerMessage->setText("Password must be at least 4 characters.");
        return;
    }
    
    if (password != passwordConfirm) {
        m_registerMessage->setText("Passwords do not match.");
        m_registerPasswordConfirm->clear();
        m_registerPasswordConfirm->setFocus();
        return;
    }
    
    UserManager& manager = UserManager::instance();
    if (manager.userExists(username)) {
        m_registerMessage->setText("Username already exists. Please choose another.");
        return;
    }
    
    // Create user
    if (manager.createUser(username, password, displayName)) {
        // Auto-login
        if (manager.authenticateUser(username, password)) {
            QMessageBox::information(this, "Success", 
                "Account created successfully! You are now logged in.");
            accept();
        }
    } else {
        m_registerMessage->setText("Failed to create account. Please try again.");
    }
}

void LoginDialog::onCancelClicked() {
    reject();
}