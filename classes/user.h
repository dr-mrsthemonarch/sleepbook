#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>

class User {
public:
    User();
    User(const QString& username, const QString& passwordHash, const QString& displayName = "");
    
    QString getUsername() const { return m_username; }
    QString getPasswordHash() const { return m_passwordHash; }
    QString getDisplayName() const { return m_displayName; }
    QDateTime getCreatedDate() const { return m_createdDate; }
    QDateTime getLastLogin() const { return m_lastLogin; }
    
    void setUsername(const QString& username) { m_username = username; }
    void setPasswordHash(const QString& hash) { m_passwordHash = hash; }
    void setDisplayName(const QString& name) { m_displayName = name; }
    void setLastLogin(const QDateTime& dt) { m_lastLogin = dt; }
    
    bool verifyPassword(const QString& password) const;
    
    // Serialization
    QString serialize() const;
    static User deserialize(const QString& str);
    
    // Hash password
    static QString hashPassword(const QString& password);
    
private:
    QString m_username;
    QString m_passwordHash;
    QString m_displayName;
    QDateTime m_createdDate;
    QDateTime m_lastLogin;
};

#endif // USER_H