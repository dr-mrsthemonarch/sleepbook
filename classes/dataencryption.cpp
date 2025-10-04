#include "dataencryption.h"
#include <QCryptographicHash>
#include <QFile>
#include <QDataStream>

QByteArray DataEncryption::deriveKey(const QString& password) {
    // Use SHA256 to create a 32-byte key from password
    QByteArray key = QCryptographicHash::hash(
        password.toUtf8(),
        QCryptographicHash::Sha256
    );
    return key;
}

QByteArray DataEncryption::encrypt(const QByteArray& data, const QString& password) {
    QByteArray key = deriveKey(password);
    QByteArray encrypted;
    encrypted.resize(data.size());
    
    // XOR encryption with repeating key
    for (int i = 0; i < data.size(); ++i) {
        encrypted[i] = data[i] ^ key[i % key.size()];
    }
    
    return encrypted;
}

QByteArray DataEncryption::decrypt(const QByteArray& data, const QString& password) {
    // XOR is symmetric, so decrypt is the same as encrypt
    return encrypt(data, password);
}

bool DataEncryption::saveEncrypted(const QString& filename, const QByteArray& data, const QString& password) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QByteArray encrypted = encrypt(data, password);
    
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);
    
    // Write magic number to identify our format
    out << quint32(0x534C5050); // "SLPP" (Sleep Tracker)
    // Write version
    out << quint32(1);
    // Write encrypted data
    out << encrypted;
    
    file.close();
    return true;
}

QByteArray DataEncryption::loadEncrypted(const QString& filename, const QString& password) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);
    
    quint32 magic, version;
    in >> magic >> version;
    
    // Verify magic number
    if (magic != 0x534C5050) {
        file.close();
        return QByteArray();
    }
    
    QByteArray encrypted;
    in >> encrypted;
    
    file.close();
    
    return decrypt(encrypted, password);
}