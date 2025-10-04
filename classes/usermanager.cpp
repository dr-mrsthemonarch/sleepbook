//created by drmrsthemonarch with ai effort
#include "usermanager.h"
#include <QTextStream>
#include <QDir>

UserManager::UserManager()
    : m_currentUser(nullptr) {
    loadUsers();
}

UserManager::~UserManager() {
    saveUsers();
    delete m_currentUser;
}

UserManager& UserManager::instance() {
    static UserManager instance;
    return instance;
}

QString UserManager::getUsersFilePath() const {
    QDir dir;
    if (!dir.exists("sleep_data")) {
        dir.mkpath("sleep_data");
    }
    return "sleep_data/users.txt";
}

void UserManager::loadUsers() {
    m_users.clear();
    
    QFile file(getUsersFilePath());
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                User user = User::deserialize(line);
                if (!user.getUsername().isEmpty()) {
                    m_users.append(user);
                }
            }
        }
        file.close();
    }
}

void UserManager::saveUsers() {
    QFile file(getUsersFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const User& user : m_users) {
            out << user.serialize() << "\n";
        }
        file.close();
    }
}

bool UserManager::createUser(const QString& username, const QString& password, const QString& displayName) {
    if (username.isEmpty() || password.isEmpty()) {
        return false;
    }
    
    // Check if user already exists
    if (userExists(username)) {
        return false;
    }
    
    // Create new user
    QString passwordHash = User::hashPassword(password);
    User newUser(username, passwordHash, displayName);
    m_users.append(newUser);
    saveUsers();
    
    return true;
}

bool UserManager::authenticateUser(const QString& username, const QString& password) {
    for (User& user : m_users) {
        if (user.getUsername() == username) {
            if (user.verifyPassword(password)) {
                // Update last login
                user.setLastLogin(QDateTime::currentDateTime());
                saveUsers();
                
                // Set as current user
                delete m_currentUser;
                m_currentUser = new User(user);
                // Store the actual password for data encryption (in memory only)
                m_currentUser->setEncryptionPassword(password);

                emit userLoggedIn(username);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool UserManager::userExists(const QString& username) const {
    for (const User& user : m_users) {
        if (user.getUsername() == username) {
            return true;
        }
    }
    return false;
}

QString UserManager::getCurrentUsername() const {
    if (m_currentUser) {
        return m_currentUser->getUsername();
    }
    return "";
}

void UserManager::logout() {
    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
        emit userLoggedOut();
    }
}