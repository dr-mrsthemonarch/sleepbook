//created by drmrsthemonarch with ai effort
#include "datapathmanager.h"

DataPathManager& DataPathManager::instance()
{
    static DataPathManager instance;
    return instance;
}

QString DataPathManager::getAppDataPath() const
{
    if (m_appDataPath.isEmpty()) {
        // Use AppDataLocation for cross-platform compatibility
        // On macOS: ~/Library/Application Support/sleepbook
        // On Windows: C:/Users/Username/AppData/Roaming/sleepbook
        // On Linux: ~/.local/share/sleepbook
        m_appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(m_appDataPath);
    }
    return m_appDataPath;
}

QString DataPathManager::getSleepDataPath() const
{
    if (m_sleepDataPath.isEmpty()) {
        m_sleepDataPath = getAppDataPath() + "/sleep_data";
        QDir().mkpath(m_sleepDataPath);
    }
    return m_sleepDataPath;
}

QString DataPathManager::getUserDataPath(const QString& username) const
{
    QString userPath = getSleepDataPath() + "/" + username;
    QDir().mkpath(userPath);
    return userPath;
}

QString DataPathManager::getUsersFilePath() const
{
    return getSleepDataPath() + "/users.txt";
}