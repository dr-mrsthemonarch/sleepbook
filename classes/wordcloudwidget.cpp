//created by drmrsthemonarch with ai effort
#include "wordcloudwidget.h"
#include <QPaintEvent>
#include <QFontMetrics>
#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <algorithm>

WordCloudWidget::WordCloudWidget(QWidget *parent)
    : QWidget(parent)
    , m_minFontSize(12)
    , m_maxFontSize(48)
    , m_maxWords(100)
    , m_maxFrequency(1)
    , m_minFrequency(1)
    , m_needsLayout(false)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);

    // Default color scheme - blues and greens for a calming sleep theme
    m_colorScheme = {
        QColor(70, 130, 180),   // Steel Blue
        QColor(100, 149, 237),  // Cornflower Blue
        QColor(72, 61, 139),    // Dark Slate Blue
        QColor(106, 90, 205),   // Slate Blue
        QColor(123, 104, 238),  // Medium Slate Blue
        QColor(147, 112, 219),  // Medium Purple
        QColor(138, 43, 226),   // Blue Violet
        QColor(75, 0, 130),     // Indigo
        QColor(25, 25, 112),    // Midnight Blue
        QColor(0, 100, 0),      // Dark Green
        QColor(34, 139, 34),    // Forest Green
        QColor(50, 205, 50),    // Lime Green
    };

    m_layoutTimer = new QTimer(this);
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(100);
    connect(m_layoutTimer, &QTimer::timeout, this, &WordCloudWidget::calculateLayout);
}

void WordCloudWidget::setWordFrequencies(const QMap<QString, int>& frequencies) {
    m_wordFrequencies = frequencies;
    m_words.clear();

    if (frequencies.isEmpty()) {
        update();
        return;
    }

    // Find min and max frequencies
    m_maxFrequency = 0;
    m_minFrequency = INT_MAX;

    for (auto it = frequencies.begin(); it != frequencies.end(); ++it) {
        if (it.value() > m_maxFrequency) {
            m_maxFrequency = it.value();
        }
        if (it.value() < m_minFrequency) {
            m_minFrequency = it.value();
        }
    }

    // Create sorted list of words by frequency (descending)
    QList<QPair<QString, int>> sortedWords;
    for (auto it = frequencies.begin(); it != frequencies.end(); ++it) {
        sortedWords.append({it.key(), it.value()});
    }

    std::sort(sortedWords.begin(), sortedWords.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });

    // Take only the top words
    int wordCount = qMin(m_maxWords, sortedWords.size());
    for (int i = 0; i < wordCount; ++i) {
        WordInfo word(sortedWords[i].first, sortedWords[i].second);
        m_words.append(word);
    }

    m_needsLayout = true;
    m_layoutTimer->start();
}

void WordCloudWidget::calculateLayout() {
    if (m_words.isEmpty()) {
        return;
    }

    // If widget hasn't been shown yet, defer layout
    if (size().isEmpty() || width() <= 0 || height() <= 0) {
        m_needsLayout = true;
        return;
    }

    calculateWordSizes();
    placeWordsSpiral();
    m_needsLayout = false;
    update();
}

void WordCloudWidget::calculateWordSizes() {
    if (m_maxFrequency == m_minFrequency) {
        // All words have the same frequency
        int fontSize = (m_minFontSize + m_maxFontSize) / 2;
        for (WordInfo& word : m_words) {
            word.font = QFont("Arial", fontSize, QFont::Bold);
            word.color = getRandomColor();
        }
        return;
    }

    for (WordInfo& word : m_words) {
        // Calculate font size based on frequency
        double ratio = static_cast<double>(word.frequency - m_minFrequency) /
                      (m_maxFrequency - m_minFrequency);
        int fontSize = m_minFontSize + static_cast<int>(ratio * (m_maxFontSize - m_minFontSize));

        word.font = QFont("Arial", fontSize, QFont::Bold);
        word.color = getRandomColor();
        word.placed = false;
    }
}

QColor WordCloudWidget::getRandomColor() const {
    if (m_colorScheme.isEmpty()) {
        return QColor(Qt::blue);
    }

    int index = QRandomGenerator::global()->bounded(m_colorScheme.size());
    return m_colorScheme[index];
}

void WordCloudWidget::placeWordsSpiral() {
    QPointF center(width() / 2.0, height() / 2.0);

    for (WordInfo& word : m_words) {
        QFontMetrics fm(word.font);
        QRect textRect = fm.boundingRect(word.text);
        word.boundingRect = textRect;

        // Try to place the word using spiral algorithm
        bool placed = false;
        const int maxSteps = 1000;

        for (int step = 0; step < maxSteps && !placed; ++step) {
            QPointF pos = getSpiralPoint(step, center);

            // Center the text at this position
            word.position = QPointF(pos.x() - textRect.width() / 2.0,
                                   pos.y() + textRect.height() / 2.0);

            // Update bounding rect with the actual position
            word.boundingRect = QRect(word.position.x(), word.position.y() - textRect.height(),
                                     textRect.width(), textRect.height());

            // Check if this position is within bounds and doesn't collide
            if (word.boundingRect.left() >= 0 &&
                word.boundingRect.right() <= width() &&
                word.boundingRect.top() >= 0 &&
                word.boundingRect.bottom() <= height()) {

                // Check collision with already placed words
                bool collision = false;
                for (const WordInfo& otherWord : m_words) {
                    if (&otherWord != &word && otherWord.placed) {
                        if (getInflatedRect(word.boundingRect, 5).intersects(
                                getInflatedRect(otherWord.boundingRect, 5))) {
                            collision = true;
                            break;
                        }
                    }
                }

                if (!collision) {
                    word.placed = true;
                    placed = true;
                }
            }
        }

        if (!placed) {
            // If we couldn't place the word, hide it
            word.placed = false;
        }
    }
}

QPointF WordCloudWidget::getSpiralPoint(int step, const QPointF& center) const {
    double angle = step * 0.1;
    double radius = step * 2.0;

    double x = center.x() + radius * cos(angle);
    double y = center.y() + radius * sin(angle);

    return QPointF(x, y);
}

QRect WordCloudWidget::getInflatedRect(const QRect& rect, int inflation) const {
    return rect.adjusted(-inflation, -inflation, inflation, inflation);
}

void WordCloudWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    if (m_words.isEmpty()) {
        // Draw placeholder text
        painter.setPen(Qt::lightGray);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(rect(), Qt::AlignCenter,
                        "No data available.\nAdd some sleep entries with notes to generate a word cloud.");
        return;
    }

    // Draw words
    for (const WordInfo& word : m_words) {
        if (!word.placed) continue;

        painter.setFont(word.font);

        // Highlight hovered word
        if (word.text == m_hoveredWord) {
            QColor highlightColor = word.color.lighter(150);
            painter.setPen(highlightColor);
        } else {
            painter.setPen(word.color);
        }

        painter.drawText(word.position, word.text);
    }
}

void WordCloudWidget::mousePressEvent(QMouseEvent* event) {
    for (const WordInfo& word : m_words) {
        if (word.placed && word.boundingRect.contains(event->pos())) {
            emit wordClicked(word.text, word.frequency);
            break;
        }
    }

    QWidget::mousePressEvent(event);
}

void WordCloudWidget::mouseMoveEvent(QMouseEvent* event) {
    QString newHoveredWord;

    for (const WordInfo& word : m_words) {
        if (word.placed && word.boundingRect.contains(event->pos())) {
            newHoveredWord = word.text;
            QToolTip::showText(event->globalPos(),
                              QString("%1 (appears %2 times)").arg(word.text).arg(word.frequency));
            emit wordHovered(word.text, word.frequency);
            break;
        }
    }

    if (newHoveredWord != m_hoveredWord) {
        m_hoveredWord = newHoveredWord;
        update();
    }

    if (newHoveredWord.isEmpty()) {
        QToolTip::hideText();
    }

    QWidget::mouseMoveEvent(event);
}

void WordCloudWidget::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)

    if (!m_words.isEmpty()) {
        m_needsLayout = true;
        m_layoutTimer->start();
    }
}

QStringList WordCloudWidget::getStopWords() {
    return {"a", "about", "above", "across", "after", "again", "against", "all", "almost",
    "along", "already", "also", "although", "always", "am", "among", "an", "and",
    "another", "any", "anyone", "anything", "anywhere", "are", "aren't", "around",
    "as", "at", "away", "back", "be", "became", "because", "become", "becomes",
    "been", "before", "being", "below", "between", "both", "but", "by", "came",
    "can", "cannot", "could", "couldn't", "did", "didn't", "do", "does", "doesn't",
    "doing", "don't", "done", "down", "during", "each", "eight", "either", "else",
    "enough", "even", "ever", "every", "everyone", "everything", "everywhere",
    "few", "find", "first", "five", "for", "former", "found", "four", "from",
    "further", "get", "give", "go", "gone", "got", "great", "had", "hadn't",
    "has", "hasn't", "have", "haven't", "having", "he", "he'd", "he'll", "he's",
    "her", "here", "here's", "hers", "herself", "him", "himself", "his", "how",
    "how's", "however", "i", "i'd", "i'll", "i'm", "i've", "if", "in", "indeed",
    "into", "is", "isn't", "it", "it's", "its", "itself", "just", "keep", "kind",
    "knew", "know", "known", "large", "last", "later", "least", "let", "let's",
    "like", "likely", "little", "long", "look", "made", "make", "makes", "many",
    "may", "me", "might", "mine", "more", "most", "mostly", "much", "must",
    "my", "myself", "name", "near", "need", "never", "new", "next", "no", "none",
    "nor", "not", "nothing", "now", "of", "off", "often", "on", "once", "one",
    "only", "onto", "or", "other", "others", "our", "ours", "ourselves", "out",
    "over", "own", "part", "perhaps", "place", "put", "rather", "really", "right",
    "said", "same", "saw", "say", "says", "see", "seem", "seemed", "seeming",
    "seems", "set", "shall", "she", "she'd", "she'll", "she's", "should",
    "shouldn't", "show", "since", "six", "small", "so", "some", "someone",
    "something", "sometimes", "somewhere", "still", "such", "take", "ten",
    "than", "that", "that's", "the", "their", "theirs", "them", "themselves",
    "then", "there", "there's", "these", "they", "they'd", "they'll", "they're",
    "they've", "thing", "things", "this", "those", "though",
    "through", "thus", "time", "to", "too", "took", "toward", "turn", "two",
    "under", "until", "up", "upon", "us", "use", "used", "using", "very", "was",
    "wasn't", "way", "we", "we'd", "we'll", "we're", "we've", "well", "went",
    "were", "weren't", "what", "what's", "when", "whenever", "where", "where's",
    "whether", "which", "while", "who", "who's", "whom", "whose", "why", "why's",
    "will", "with", "within", "without", "won't", "would", "wouldn't", "yes",
    "yet", "you", "you'd", "you'll", "you're", "you've", "your", "yours",
    "yourself", "yourselves","able", "across", "ahead", "ago", "ain't", "almost", "alone", "alongside",
    "already", "alright", "although", "always", "anybody", "anyhow", "anytime",
    "anyway", "anyways", "around", "ask", "away", "awesome", "backward", "basically", "became", "becoming", "beforehand", "behind", "beside",
    "besides", "beyond", "big", "bit", "both", "bottom", "bring",
    "brought", "came", "cannot", "cause", "certain", "clearly", "come", "comes",
    "consider", "contain", "contains", "continue", "day", "decided", "definitely",
    "doing", "done", "downward", "during", "each", "early", "either", "elsewhere",
    "enough", "especially", "etc", "ever", "everybody", "everyone", "everything",
    "everywhere", "example", "except", "far", "few", "finally", "followed",
    "following", "forward", "full", "furthermore", "gave", "getting", "give",
    "goes", "gonna", "gotten", "greatly", "guess", "hadn't", "happened", "hardly",
    "having", "hello", "help", "hey", "highly", "hopefully", "however",
    "i.e.", "idea", "indeed", "instead", "it’d", "it’ll", "keep", "kinda", "knows",
    "later", "least", "less", "let", "lets", "likely", "little", "lot", "lots",
    "mainly", "make", "maybe", "mean", "means", "meanwhile", "merely", "might",
    "mine", "minute", "month", "mostly", "move", "much", "must", "nearly",
    "necessary", "neither", "next", "nobody", "none", "nonetheless", "normally",
    "nothing", "nowhere", "obviously", "often", "okay", "old", "once", "one’s",
    "oops", "option", "otherwise", "outside", "overall", "particular", "perhaps",
    "place", "plus", "pretty", "probably", "quite", "rather", "really", "reason",
    "recent", "recently", "same", "several", "shall", "short", "show", "simply",
    "since", "slightly", "somebody", "somehow", "sometime", "somewhat", "somewhere",
    "sort", "soon", "sorry", "stuff", "such", "suppose", "sure", "take", "taken",
    "thing", "though", "throughout", "together", "towards", "try", "trying", "turns",
    "twelve", "twenty", "type", "uh", "unless", "unlike", "until", "upon", "useful",
    "usually", "via", "want", "was", "wasn’t", "way", "week", "well", "whatever",
    "whenever", "wherever", "whether", "whose", "within", "without", "won’t",
    "work", "world", "worse", "worst", "worth", "yeah", "year", "yes", "yet","above", "add", "added", "adding", "afterward", "against", "ahead", "allow",
    "allows", "almost", "already", "although", "amongst", "apart", "appear",
    "appeared", "appearing", "appears", "approximately", "aren’t", "around",
    "aside", "associated", "available", "away", "backwards", "became", "becomes",
    "began", "begin", "beginning", "begins", "behind", "believe", "best", "beyond", "bottom", "broad", "built", "call", "called", "cannot",
    "case", "certainly", "change", "changed", "changes", "clever", "close",
    "closed", "comes", "completely", "considered", "contain", "containing",
    "contains", "corresponding", "couldn’t", "currently", "daily", "deal", "dealt",
    "deep", "definite", "described", "despite", "detail", "details", "developed",
    "different", "done", "downwards", "due", "during", "earlier", "eight",
    "eighty", "either", "eleven", "empty", "ended", "ending", "enough", "entire",
    "especially", "etcetera", "evening", "everywhere", "exactly", "example",
    "excepted", "excepting", "excluding", "expected", "fairly", "fifteen",
    "fifty", "finally", "follow", "follows", "former", "formerly", "forty",
    "forward", "found", "four", "full", "furthered", "gets", "give", "given",
    "giving", "goes", "going", "gone", "good", "got", "great", "half", "happens",
    "hard", "hasn’t", "having", "hence", "hereafter", "hereby", "herein",
    "hereupon", "high", "hopefully", "however", "hundred", "i.e.", "ignored",
    "immediate", "importance", "important", "improve", "improved", "including",
    "increasing", "indeed", "indicate", "indicated", "indicates", "inner",
    "inside", "instead", "interest", "interested", "interesting", "involved",
    "itself", "joined", "keep", "keeps", "knew", "known", "knows", "large",
    "largely", "later", "least", "less", "likely", "limited", "longer", "looking",
    "looks", "lot", "main", "mainly", "make", "makes", "making", "many", "maybe",
    "mean", "means", "member", "members", "mentioned", "merely", "middle",
    "mightn’t", "mine", "mostly", "move", "much", "mustn’t", "name", "namely",
    "necessary", "need", "needed", "needing", "neither", "next", "ninety",
    "nobody", "non", "none", "nonetheless", "normally", "nothing", "notice",
    "nowadays", "obtain", "obtained", "obviously", "occasionally", "odd",
    "often", "old", "once", "one’s", "onto", "open", "opened", "opening", "opens",
    "opposite", "order", "ordered", "other", "others", "otherwise", "outside",
    "overall", "owning", "page", "pages", "perhaps", "placed", "please",
    "plenty", "plus", "point", "pointed", "points", "possible", "present",
    "presented", "presents", "probably", "provide", "provided", "provides",
    "putting", "quickly", "quite", "rather", "really", "recent", "recently",
    "related", "relatively", "require", "required", "requires", "result",
    "resulted", "resulting", "results", "round", "run", "running", "same",
    "saying", "says", "second", "see", "seem", "seems", "seen", "several",
    "shortly", "showed", "shown", "shows", "side", "significant", "similar",
    "similarly", "simply", "slight", "slightly", "small", "somebody", "somehow",
    "someone", "something", "sometime", "sometimes", "somewhat", "somewhere",
    "soon", "sorry", "special", "specific", "specifically", "spent", "stated",
    "states", "still", "stop", "strong", "strongly", "such", "sufficient",
    "taking", "tell", "tells", "term", "terms", "thank", "thanks", "thereafter",
    "thereby", "therefore", "thick", "thin", "thing", "things", "think",
    "thirty", "thorough", "thoroughly", "though", "thought", "three", "through",
    "throughout", "thru", "thus", "together", "told", "top", "toward", "towards",
    "twelve", "twenty", "two", "underneath", "understanding", "unfortunately",
    "unless", "unlike", "unlikely", "until", "upon", "usefully", "usefulness",
    "usually", "various", "very", "via", "wanting", "was", "wasn’t", "way",
    "week", "well", "went", "whatever", "whence", "whenever", "whereafter",
    "whereas", "whereby", "wherein", "whereupon", "wherever", "whether",
    "whichever", "whose", "wide", "will", "willing", "wish", "within", "without",
    "wonder", "word", "work", "world", "worth", "wouldn’t",
    "yeah", "year", "yes", "yet",
};
};

void WordCloudWidget::setMinimumFrequency(int minFreq) {
    m_minFrequency = minFreq;
    calculateLayout();
    update();
}

void WordCloudWidget::setWords(const QMap<QString, int>& wordFreqs) {
    wordFrequencies = wordFreqs;

    // Find maximum frequency for scaling
    maxFrequencyValue = 1;
    for (auto it = wordFrequencies.begin(); it != wordFrequencies.end(); ++it) {
        if (it.value() > maxFrequencyValue) {
            maxFrequencyValue = it.value();
        }
    }

    calculateLayout();
    update();
}

void WordCloudWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    if (m_needsLayout && !m_words.isEmpty()) {
        m_layoutTimer->start();
    }
}

