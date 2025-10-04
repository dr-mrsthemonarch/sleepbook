#include "symptomwidget.h"

SymptomWidget::SymptomWidget(const Symptom& symptom, QWidget* parent)
    : QWidget(parent), m_symptom(symptom), m_countSpinBox(nullptr),
      m_quantitySpinBox(nullptr), m_unitLabel(nullptr) {
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    
    m_checkbox = new QCheckBox(symptom.getName(), this);
    layout->addWidget(m_checkbox);
    
    // Create appropriate input widget based on type
    switch (symptom.getType()) {
        case SymptomType::Binary:
            // Just checkbox, nothing else needed
            break;
            
        case SymptomType::Count:
            m_countSpinBox = new QSpinBox(this);
            m_countSpinBox->setMinimum(0);
            m_countSpinBox->setMaximum(99);
            m_countSpinBox->setValue(0);
            m_countSpinBox->setEnabled(false);
            m_countSpinBox->setMaximumWidth(60);
            layout->addWidget(m_countSpinBox);
            
            if (!symptom.getUnit().isEmpty()) {
                m_unitLabel = new QLabel(symptom.getUnit(), this);
                layout->addWidget(m_unitLabel);
            }
            break;
            
        case SymptomType::Quantity:
            m_quantitySpinBox = new QDoubleSpinBox(this);
            m_quantitySpinBox->setMinimum(0.0);
            m_quantitySpinBox->setMaximum(999.9);
            m_quantitySpinBox->setValue(0.0);
            m_quantitySpinBox->setDecimals(1);
            m_quantitySpinBox->setSingleStep(0.5);
            m_quantitySpinBox->setEnabled(false);
            m_quantitySpinBox->setMaximumWidth(70);
            layout->addWidget(m_quantitySpinBox);
            
            if (!symptom.getUnit().isEmpty()) {
                m_unitLabel = new QLabel(symptom.getUnit(), this);
                layout->addWidget(m_unitLabel);
            }
            break;
    }
    
    layout->addStretch();
    
    connect(m_checkbox, &QCheckBox::stateChanged, this, &SymptomWidget::onCheckboxChanged);
    
    if (m_countSpinBox) {
        connect(m_countSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &SymptomWidget::valueChanged);
    }
    if (m_quantitySpinBox) {
        connect(m_quantitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &SymptomWidget::valueChanged);
    }
}

void SymptomWidget::onCheckboxChanged(int state) {
    bool checked = (state == Qt::Checked);
    
    if (m_countSpinBox) {
        m_countSpinBox->setEnabled(checked);
        if (checked && m_countSpinBox->value() == 0) {
            m_countSpinBox->setValue(1);
        }
    }
    
    if (m_quantitySpinBox) {
        m_quantitySpinBox->setEnabled(checked);
        if (checked && m_quantitySpinBox->value() == 0.0) {
            m_quantitySpinBox->setValue(1.0);
        }
    }
    
    emit valueChanged();
}

Symptom SymptomWidget::getSymptom() const {
    Symptom s = m_symptom;
    s.setPresent(m_checkbox->isChecked());
    
    if (s.isPresent()) {
        switch (m_symptom.getType()) {
            case SymptomType::Binary:
                s.setValue(1.0);
                break;
            case SymptomType::Count:
                if (m_countSpinBox) {
                    s.setValue(m_countSpinBox->value());
                }
                break;
            case SymptomType::Quantity:
                if (m_quantitySpinBox) {
                    s.setValue(m_quantitySpinBox->value());
                }
                break;
        }
    } else {
        s.setValue(0.0);
    }
    
    return s;
}

void SymptomWidget::reset() {
    m_checkbox->setChecked(false);
    if (m_countSpinBox) {
        m_countSpinBox->setValue(0);
        m_countSpinBox->setEnabled(false);
    }
    if (m_quantitySpinBox) {
        m_quantitySpinBox->setValue(0.0);
        m_quantitySpinBox->setEnabled(false);
    }
}