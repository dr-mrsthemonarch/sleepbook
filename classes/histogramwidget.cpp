#include "histogramwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>

HistogramWidget::HistogramWidget(QWidget* parent) 
    : QWidget(parent), m_maxValue(0) {
}

void HistogramWidget::setData(const QVector<double>& values, const QVector<QString>& labels, const QColor& color, double maxValue) {
    m_values = values;
    m_labels = labels;
    m_color = color;
    m_maxValue = maxValue > 0 ? maxValue : 1.0; // Avoid division by zero
    
    update(); // Trigger repaint
}

void HistogramWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    if (m_values.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate dimensions
    int width = this->width();
    int height = this->height();
    int margin = 10;
    int chartWidth = width - 2 * margin;
    int chartHeight = height - 2 * margin;

    if (chartWidth <= 0 || chartHeight <= 0) return;

    // Calculate bar dimensions
    int barCount = m_values.size();
    int barWidth = qMax(20, chartWidth / (barCount * 2)); // Wider bars for better visibility
    int barSpacing = chartWidth / barCount;

    // Draw grid and background
    painter.fillRect(rect(), Qt::white);

    // Draw grid lines
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    for (int i = 0; i <= barCount; ++i) {
        double x = margin + i * barSpacing;
        painter.drawLine(QPointF(x, margin), QPointF(x, margin + chartHeight));
    }

    // Draw y-axis labels
    painter.setPen(Qt::black);
    QFontMetrics fm(painter.font());
    for (int i = 0; i <= 5; ++i) {
        double value = (m_maxValue * i) / 5;
        QString label = QString::number(value, 'f', 1);
        int labelWidth = fm.horizontalAdvance(label);
        int y = margin + (chartHeight * (5 - i)) / 5;
        painter.drawText(margin - labelWidth - 5, y + fm.height() / 3, label);
    }

    // Draw bars
    QColor barColor = m_color;
    QColor borderColor = m_color.darker(150);

    for (int i = 0; i < barCount; ++i) {
        double value = m_values[i];
        double normalizedHeight = (value / m_maxValue) * chartHeight;

        int x = margin + (i * barSpacing) + (barSpacing - barWidth) / 2;
        int y = margin + chartHeight - normalizedHeight;
        int barHeight = normalizedHeight;

        // Draw bar
        painter.setBrush(barColor);
        painter.setPen(QPen(borderColor, 1));
        painter.drawRect(x, y, barWidth, barHeight);

        // Draw value label on top of bar if there's space
        if (barHeight > 20) {
            painter.setPen(Qt::black);
            QString valueText = QString::number(value, 'f', 1);
            int textWidth = fm.horizontalAdvance(valueText);
            painter.drawText(x + (barWidth - textWidth) / 2, y - 5, valueText);
        }

        // Draw date label
        if (!m_labels.isEmpty() && i < m_labels.size()) {
            QString label = m_labels[i];
            int textWidth = fm.horizontalAdvance(label);
            painter.save();
            painter.translate(x + barWidth / 2, margin + chartHeight + 20);
            painter.rotate(-45);
            painter.drawText(-textWidth / 2, 0, label);
            painter.restore();
        }
    }

    // Draw axes
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(margin, margin, margin, margin + chartHeight); // y-axis
    painter.drawLine(margin, margin + chartHeight, width - margin, margin + chartHeight); // x-axis
}