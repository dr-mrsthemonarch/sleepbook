#include "symptom.h"
#include <QStringList>

Symptom::Symptom()
    : m_type(SymptomType::Binary), m_value(0.0), m_present(false) {
}

Symptom::Symptom(const QString& name, SymptomType type, const QString& unit)
    : m_name(name), m_type(type), m_unit(unit), m_value(0.0), m_present(false) {
}

QString Symptom::getColumnName() const {
    QString cleanName = m_name;
    cleanName.replace(" ", "_").replace("/", "_").replace(",", "");
    
    if (m_type == SymptomType::Binary) {
        return cleanName;
    } else {
        QString cleanUnit = m_unit;
        cleanUnit.replace(" ", "_");
        return QString("%1_%2").arg(cleanName).arg(cleanUnit);
    }
}

QString Symptom::serialize() const {
    // Format: name|type|unit
    return QString("%1|%2|%3")
        .arg(m_name)
        .arg(typeToString(m_type))
        .arg(m_unit);
}

Symptom Symptom::deserialize(const QString& str) {
    QStringList parts = str.split('|');
    if (parts.size() >= 2) {
        QString name = parts[0];
        SymptomType type = stringToType(parts[1]);
        QString unit = (parts.size() >= 3) ? parts[2] : "";
        return Symptom(name, type, unit);
    }
    return Symptom();
}

QString Symptom::typeToString(SymptomType type) {
    switch (type) {
        case SymptomType::Binary: return "Binary";
        case SymptomType::Count: return "Count";
        case SymptomType::Quantity: return "Quantity";
    }
    return "Binary";
}

SymptomType Symptom::stringToType(const QString& str) {
    if (str == "Count") return SymptomType::Count;
    if (str == "Quantity") return SymptomType::Quantity;
    return SymptomType::Binary;
}