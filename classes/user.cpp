//created by drmrsthemonarch with ai effort
#include "user.h"
#include <QCryptographicHash>
#include <QStringList>

User::User()
    : m_createdDate(QDateTime::currentDateTime()) {
}

User::User(const QString& username, const QString& passwordHash, const QString& displayName)
    : m_username(username), m_passwordHash(passwordHash), 
      m_displayName(displayName.isEmpty() ? username : displayName),
      m_createdDate(QDateTime::currentDateTime()) {
}

bool User::verifyPassword(const QString& password) const {
    return hashPassword(password) == m_passwordHash;
}

QString User::hashPassword(const QString& password) {
    QByteArray hash = QCryptographicHash::hash(
        password.toUtf8(), 
        QCryptographicHash::Sha256
    );
    return QString(hash.toHex());
}

QString User::serialize() const {
    // Format: username|passwordHash|displayName|createdDate|lastLogin
    return QString("%1|%2|%3|%4|%5")
        .arg(m_username)
        .arg(m_passwordHash)
        .arg(m_displayName)
        .arg(m_createdDate.toString(Qt::ISODate))
        .arg(m_lastLogin.toString(Qt::ISODate));
}

User User::deserialize(const QString& str) {
    QStringList parts = str.split('|');
    if (parts.size() >= 3) {
        User user;
        user.m_username = parts[0];
        user.m_passwordHash = parts[1];
        user.m_displayName = parts[2];
        
        if (parts.size() >= 4) {
            user.m_createdDate = QDateTime::fromString(parts[3], Qt::ISODate);
        }
        if (parts.size() >= 5 && !parts[4].isEmpty()) {
            user.m_lastLogin = QDateTime::fromString(parts[4], Qt::ISODate);
        }
        
        return user;
    }
    return User();
}