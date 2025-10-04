//created by drmrsthemonarch with ai effort
#ifndef DATAENCRYPTION_H
#define DATAENCRYPTION_H

#include <QByteArray>
#include <QString>

class DataEncryption {
public:
    // Encrypt/Decrypt using XOR with derived key
    static QByteArray encrypt(const QByteArray& data, const QString& password);
    static QByteArray decrypt(const QByteArray& data, const QString& password);
    
    // Generate encryption key from password
    static QByteArray deriveKey(const QString& password);
    
    // Save/Load encrypted data to/from file
    static bool saveEncrypted(const QString& filename, const QByteArray& data, const QString& password);
    static QByteArray loadEncrypted(const QString& filename, const QString& password);
};

#endif // DATAENCRYPTION_H