
#ifndef WORDCLOUDWIDGET_H
#define WORDCLOUDWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMap>
#include <QStringList>
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QToolTip>
#include <QTimer>
#include <QRandomGenerator>
#include <cmath>

struct WordInfo {
    QString text;
    int frequency;
    QFont font;
    QColor color;
    QRect boundingRect;
    QPointF position;
    bool placed;

    WordInfo() : frequency(0), placed(false) {}
    WordInfo(const QString& word, int freq)
        : text(word), frequency(freq), placed(false) {}
};

class WordCloudWidget : public QWidget {
    Q_OBJECT

public:
    explicit WordCloudWidget(QWidget *parent = nullptr);

    void setWords(const QMap<QString, int>& wordFreqs);



    void setWordFrequencies(const QMap<QString, int>& frequencies);
    void clearWords();
    void regenerateLayout();


    // Configuration
    void setMinFontSize(int size) { m_minFontSize = size; update(); }
    void setMaxFontSize(int size) { m_maxFontSize = size; update(); }
    void setMaxWords(int maxWords) { m_maxWords = maxWords; }
    void setColorScheme(const QList<QColor>& colors) { m_colorScheme = colors; update(); }
    void setMinimumFrequency(int minFreq);

    int getMinFontSize() const { return m_minFontSize; }
    int getMaxFontSize() const { return m_maxFontSize; }
    int getMaxWords() const { return m_maxWords; }

signals:
    void wordClicked(const QString& word, int frequency);
    void wordHovered(const QString& word, int frequency);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent *event);
    QStringList getStopWords() const;


private:
    void calculateLayout();
    void calculateWordSizes();
    QColor getRandomColor() const;
    bool findPositionForWord(WordInfo& word);
    bool checkCollision(const WordInfo& word, const QList<WordInfo>& placedWords) const;
    QRect getInflatedRect(const QRect& rect, int inflation) const;
    void placeWordsSpiral();
    QPointF getSpiralPoint(int step, const QPointF& center) const;

    QMap<QString, int> m_wordFrequencies;
    QList<WordInfo> m_words;
    QList<QColor> m_colorScheme;

    QMap<QString, int> wordFrequencies;
    QList<WordInfo> layoutWords;
    int minFrequency;
    int maxWords;
    int maxFrequencyValue;



    int m_minFontSize;
    int m_maxFontSize;
    int m_maxWords;
    int m_maxFrequency;
    int m_minFrequency;

    QString m_hoveredWord;
    QTimer* m_layoutTimer;
    bool m_needsLayout;
};

#endif // WORDCLOUDWIDGET_H