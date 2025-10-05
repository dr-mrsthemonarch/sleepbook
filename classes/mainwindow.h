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
#include "symptom.h"
#include "symptomwidget.h"
#include "usermanager.h"
#include "dataencryption.h"
#include "qcustomplot.h"

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
    void onDeleteHistoryEntry();
    void onPlotTypeChanged(int index);
    void onSelectAllSymptoms();
    void onDeselectAllSymptoms();
    void syncXAxes(const QCPRange& newRange);

private:
    void setupUI();
    void setupEntryTab();
    void setupHistoryTab();
    void setupStatisticsTab();
    void loadSymptoms();
    void saveSymptoms();
    void rebuildSymptomWidgets();
    bool saveEntry();
    bool saveSummaryEntry();
    QString getCurrentDataDirectory();
    QString getSymptomDataFile();
    void showAddSymptomDialog();
    void updateWindowTitle();
    void createUserToolbar();
    void loadHistoryData();
    void loadStatisticsData();
    QList<QVariantMap> loadAllEntries();
    QList<QVariantMap> filterEntriesByDateRange(const QList<QVariantMap>& entries, const QDate& start, const QDate& end);
    void plotTimeSeriesData(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms);
    void plotHistogramData(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms);
    void plotHistogramOverlay(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms);
    void plotHistogramStacked(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms);
    void plotCorrelationData(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms);

    // UI Components
    QTabWidget* tabWidget;

    // Entry tab
    QWidget* entryTab;
    QTextEdit* notesEdit;
    QScrollArea* symptomsScrollArea;
    QWidget* symptomsContainer;
    QVBoxLayout* symptomsLayout;
    QDateEdit* dateEdit;
    QTimeEdit* bedtimeEdit;
    QTimeEdit* wakeupEdit;
    QPushButton* saveButton;
    QPushButton* clearButton;
    QPushButton* addSymptomButton;

    // History tab
    QWidget* historyTab;
    QTableWidget* historyTable;
    QPushButton* deleteEntryButton;
    QPushButton* refreshHistoryButton;

    // Statistics tab
    QWidget* statisticsTab;
    QComboBox* plotTypeSelector;
    QListWidget* symptomListWidget;
    QDateEdit* startDateEdit;
    QDateEdit* endDateEdit;
    QCheckBox* allDateRangeCheckbox;
    QComboBox* histogramModeSelector;
    QCustomPlot* customPlot;
    QPushButton* generatePlotButton;
    QPushButton* selectAllButton;
    QPushButton* deselectAllButton;

    // User toolbar
    QToolBar* userToolbar;
    QLabel* userLabel;
    QPushButton* logoutButton;

    QList<Symptom> symptoms;
    QList<SymptomWidget*> symptomWidgets;
    QVector<QPointer<QCPAxis>> synchronizedXAxes;
};

#endif // MAINWINDOW_H