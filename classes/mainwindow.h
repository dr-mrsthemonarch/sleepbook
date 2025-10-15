//created by drmrsthemonarch with ai effort
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDialog>
#include <QComboBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QDateEdit>
#include <QCheckBox>
#include <QPointer>
#include <QSpinBox>
#include <QSlider>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QWidget>
#include <QTimer>
#include <QRandomGenerator>
#include "symptom.h"
#include "symptomwidget.h"
#include "usermanager.h"
#include "dataencryption.h"
#include "qcustomplot.h"
#include "wordcloudwidget.h"

class WordCloudWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private slots:
    void onSaveEntry();

    void onAddSymptom();

    void onClearForm();

    void onLogout();

    void onUserChanged();

    void onTabChanged(int index);

    void onHistoryDateSelected(int row, int column);

    void onExportHistoryToCSV();

    void onDeleteHistoryEntry();

    void onPlotTypeChanged(int index);

    void onSelectAllSymptoms();

    void onDeselectAllSymptoms();

    void syncXAxes(const QCPRange &newRange);

private:
    void setupUI();

    void loadHistogramData();

    void onHistogramSelectAllSymptoms();

    void onHistogramDeselectAllSymptoms();

    void exportHistoryToCSV(const QString& filename, const QList<QVariantMap>& entries);

    void setupEntryTab();

    void setupHistoryTab();

    void setupHistogramTab();

    void setupStatisticsTab();

    void setupWordCloudTab();

    void loadSymptoms();

    void saveSymptoms();

    void rebuildSymptomWidgets();

    bool saveEntry();

    bool saveSummaryEntry();

    static QString getCurrentDataDirectory();

    static QString getSymptomDataFile();

    void showAddSymptomDialog();

    void updateWindowTitle();

    void createUserToolbar();

    void loadHistoryData();

    void loadStatisticsData();

    void loadWordCloudData();

    QList<QVariantMap> loadAllEntries();

    QList<QVariantMap> filterEntriesByDateRange(const QList<QVariantMap> &entries, const QDate &start,
                                                const QDate &end);

    void plotTimeSeriesData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms);

    void plotHistogramData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms);

    void plotHistogramOverlay(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms);

    void initializeHistogram();

    void showHistogramContextMenu(const QPoint &pos);

    void resetHistogramZoom();

    void plotHistogramStacked(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms);

    void plotCorrelationData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms);

    bool updateSummaryEntry(const QDate &date, double duration, const QList<QPair<QString, double>> &symptomData);

    // Word cloud helper functions
    QMap<QString, int> extractWordFrequencies(const QList<QVariantMap>& entries);
    QStringList tokenizeText(const QString& text);
    QStringList getStopWords();


    // UI Components
    QTabWidget *tabWidget;

    // Entry tab
    QWidget *entryTab;
    QTextEdit *notesEdit;
    QScrollArea *symptomsScrollArea;
    QWidget *symptomsContainer;
    QVBoxLayout *symptomsLayout;
    QDateEdit *dateEdit;
    QTimeEdit *bedtimeEdit;
    QTimeEdit *wakeupEdit;
    QPushButton *saveButton;
    QPushButton *clearButton;
    QPushButton *addSymptomButton;

    // History tab
    QWidget *historyTab;
    QTableWidget *historyTable;
    QPushButton *deleteEntryButton;
    QPushButton *refreshHistoryButton;


    // Statistics tab
    QWidget *statisticsTab;
    QComboBox *plotTypeSelector;
    QListWidget *symptomListWidget;
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
    QCheckBox *allDateRangeCheckbox;
    QComboBox *histogramModeSelector;
    QCustomPlot *customPlot;
    QPushButton *generatePlotButton;
    QPushButton *selectAllButton;
    QPushButton *deselectAllButton;

    // Histogram Tab members
    QWidget *histogramTab;
    QCheckBox *histogramAllDateRangeCheckbox;
    QDateEdit *histogramStartDateEdit;
    QDateEdit *histogramEndDateEdit;
    QListWidget *histogramSymptomListWidget;
    QPushButton *histogramSelectAllButton;
    QPushButton *histogramDeselectAllButton;
    QPushButton *generateHistogramButton;
    QCustomPlot *histogramCustomPlot;

    // Word Cloud tab
    QWidget *wordCloudTab;
    WordCloudWidget *wordCloudWidget;
    QCheckBox *wordCloudAllDateRangeCheckbox;
    QDateEdit *wordCloudStartDateEdit;
    QDateEdit *wordCloudEndDateEdit;
    QSpinBox *wordCloudMaxWordsSpinBox;
    QSpinBox *wordCloudMinFontSpinBox;
    QSpinBox *wordCloudMaxFontSpinBox;
    QPushButton *wordCloudGenerateButton;
    QPushButton *wordCloudRegenerateButton;
    QPushButton *generateWordCloudButton;
    QLabel *wordCloudStatsLabel;
    QSpinBox *minWordFrequencySpinBox;
    QSpinBox *maxWordsSpinBox;
    QLabel *wordCountLabel;

    // User toolbar
    QToolBar *userToolbar;
    QLabel *userLabel;
    QPushButton *logoutButton;

    QList<Symptom> symptoms;
    QList<SymptomWidget *> symptomWidgets;
    QVector<QPointer<QCPAxis> > synchronizedXAxes;

    QList<QVariantMap> loadSummaryData();
    QStringList parseCSVLine(const QString& line);
};

#endif // MAINWINDOW_H