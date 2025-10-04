//created by drmrsthemonarch with ai effort
#ifndef SYMPTOMWIDGET_H
#define SYMPTOMWIDGET_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include "symptom.h"

class SymptomWidget : public QWidget {
    Q_OBJECT

public:
    explicit SymptomWidget(const Symptom& symptom, QWidget* parent = nullptr);

    Symptom getSymptom() const;
    void reset();

    signals:
        void valueChanged();

private slots:
    void onCheckboxChanged(int state);

private:
    Symptom m_symptom;
    QCheckBox* m_checkbox;
    QSpinBox* m_countSpinBox;
    QDoubleSpinBox* m_quantitySpinBox;
    QLabel* m_unitLabel;
};

#endif // SYMPTOMWIDGET_H