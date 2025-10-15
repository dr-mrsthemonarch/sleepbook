//created by drmrsthemonarch with ai effort
#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QColor>

class HistogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    
    void setData(const QVector<double>& values, const QVector<QString>& labels, 
                 const QColor& color = Qt::blue, double maxValue = 0);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_values;
    QVector<QString> m_labels;
    QColor m_color;
    double m_maxValue;
};

#endif // HISTOGRAMWIDGET_H