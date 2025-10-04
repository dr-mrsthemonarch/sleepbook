//created by drmrsthemonarch with ai effort
#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include "user.h"

class UserManager : public QObject {
    Q_OBJECT

public:
    static UserManager& instance();

    // User management
    bool createUser(const QString& username, const QString& password, const QString& displayName = "");
    bool authenticateUser(const QString& username, const QString& password);
    bool userExists(const QString& username) const;

    // Current user
    bool isLoggedIn() const { return m_currentUser != nullptr; }
    User* getCurrentUser() const { return m_currentUser; }
    QString getCurrentUsername() const;
    void logout();

    // Data management
    void loadUsers();
    void saveUsers();

    signals:
        void userLoggedIn(const QString& username);
    void userLoggedOut();

private:
    UserManager();
    ~UserManager();
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    QString getUsersFilePath() const;

    QList<User> m_users;
    User* m_currentUser;
};

#endif // USERMANAGER_H