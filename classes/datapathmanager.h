//created by drmrsthemonarch with ai effort
#ifndef DATAPATHMANAGER_H
#define DATAPATHMANAGER_H

#include <QString>
#include <QDir>
#include <QStandardPaths>

class DataPathManager
{
public:
    static DataPathManager& instance();
    
    QString getAppDataPath() const;
    QString getSleepDataPath() const;
    QString getUserDataPath(const QString& username) const;
    QString getUsersFilePath() const;
    
private:
    DataPathManager() = default;
    ~DataPathManager() = default;
    
    mutable QString m_appDataPath;
    mutable QString m_sleepDataPath;
};

#endif // DATAPATHMANAGER_H