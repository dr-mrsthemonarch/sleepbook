#ifndef SYMPTOM_H
#define SYMPTOM_H

#include <QString>
#include <QMetaType>

enum class SymptomType {
    Binary,    // Yes/No checkbox
    Count,     // Integer count (0, 1, 2, 3...)
    Quantity   // Decimal with units
};

Q_DECLARE_METATYPE(SymptomType)

class Symptom {
public:
    Symptom();
    Symptom(const QString& name, SymptomType type, const QString& unit = "");
    
    QString getName() const { return m_name; }
    SymptomType getType() const { return m_type; }
    QString getUnit() const { return m_unit; }
    double getValue() const { return m_value; }
    bool isPresent() const { return m_present; }
    
    void setName(const QString& name) { m_name = name; }
    void setType(SymptomType type) { m_type = type; }
    void setUnit(const QString& unit) { m_unit = unit; }
    void setValue(double value) { m_value = value; }
    void setPresent(bool present) { m_present = present; }
    
    // Get column name for CSV
    QString getColumnName() const;
    
    // Serialize to/from string for saving
    QString serialize() const;
    static Symptom deserialize(const QString& str);
    
    // Get type as string
    static QString typeToString(SymptomType type);
    static SymptomType stringToType(const QString& str);
    
private:
    QString m_name;
    SymptomType m_type;
    QString m_unit;
    double m_value;
    bool m_present;
};

#endif // SYMPTOM_H