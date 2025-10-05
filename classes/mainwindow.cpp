#include "mainwindow.h"
#include <QToolBar>
#include <QDataStream>
#include <QBuffer>
#include "logindialog.h"
#include "histogramwidget.h"
#include "datapathmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    createUserToolbar();

    // Load symptoms only after UI is set up and user is logged in
    if (UserManager::instance().isLoggedIn()) {
        loadSymptoms();
        rebuildSymptomWidgets();
    }

    updateWindowTitle();
}

MainWindow::~MainWindow() {
    if (UserManager::instance().isLoggedIn()) {
        saveSymptoms();
    }
}

void MainWindow::setupUI() {
    setWindowTitle("Sleep Quality Tracker");
    resize(1200, 800); // Slightly wider to accommodate more tabs

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Create tab widget
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);

    // Create tabs
    setupEntryTab();
    setupHistoryTab();
    setupStatisticsTab();
    setupHistogramTab(); // New tab for stacked histograms

    tabWidget->addTab(entryTab, "New Entry");
    tabWidget->addTab(historyTab, "History");
    tabWidget->addTab(statisticsTab, "Statistics");
    tabWidget->addTab(histogramTab, "Histograms"); // New tab

    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

void MainWindow::loadHistogramData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    // Update symptom list if empty
    if (histogramSymptomListWidget->count() == 0) {
        QListWidgetItem *sleepItem = new QListWidgetItem("Sleep Duration");
        sleepItem->setFlags(sleepItem->flags() | Qt::ItemIsUserCheckable);
        sleepItem->setCheckState(Qt::Checked);
        histogramSymptomListWidget->addItem(sleepItem);

        for (const Symptom &s: symptoms) {
            QListWidgetItem *item = new QListWidgetItem(s.getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            histogramSymptomListWidget->addItem(item);
        }
    }

    // Get selected symptoms
    QStringList selectedSymptoms;
    for (int i = 0; i < histogramSymptomListWidget->count(); ++i) {
        QListWidgetItem *item = histogramSymptomListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedSymptoms.append(item->text());
        }
    }

    if (selectedSymptoms.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select at least one symptom or metric to plot.");
        return;
    }

    // Load all entries
    QList<QVariantMap> entries = loadAllEntries();

    if (entries.isEmpty()) {
        histogramCustomPlot->clearGraphs();
        histogramCustomPlot->clearPlottables();
        histogramCustomPlot->replot();
        QMessageBox::information(this, "No Data", "No sleep data available for analysis.");
        return;
    }

    // Filter by date range if needed
    if (!histogramAllDateRangeCheckbox->isChecked()) {
        entries = filterEntriesByDateRange(entries, histogramStartDateEdit->date(), histogramEndDateEdit->date());
        if (entries.isEmpty()) {
            QMessageBox::information(this, "No Data", "No data in selected date range.");
            return;
        }
    }

    // Sort by date
    std::sort(entries.begin(), entries.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["date"].toDate() < b["date"].toDate();
    });

    // Generate stacked histogram
    plotHistogramStacked(entries, selectedSymptoms);
}

void MainWindow::onHistogramSelectAllSymptoms() {
    for (int i = 0; i < histogramSymptomListWidget->count(); ++i) {
        histogramSymptomListWidget->item(i)->setCheckState(Qt::Checked);
    }
}

void MainWindow::onHistogramDeselectAllSymptoms() {
    for (int i = 0; i < histogramSymptomListWidget->count(); ++i) {
        histogramSymptomListWidget->item(i)->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::setupHistogramTab() {
    histogramTab = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(histogramTab);

    // Left panel - controls
    QVBoxLayout *controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Stacked Histogram Configuration");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(titleLabel);

    // Date range
    controlLayout->addWidget(new QLabel("Date Range:"));
    histogramAllDateRangeCheckbox = new QCheckBox("Use all data");
    histogramAllDateRangeCheckbox->setChecked(true);
    controlLayout->addWidget(histogramAllDateRangeCheckbox);

    QHBoxLayout *startDateLayout = new QHBoxLayout();
    startDateLayout->addWidget(new QLabel("From:"));
    histogramStartDateEdit = new QDateEdit();
    histogramStartDateEdit->setDate(QDate::currentDate().addMonths(-1));
    histogramStartDateEdit->setCalendarPopup(true);
    histogramStartDateEdit->setEnabled(false);
    startDateLayout->addWidget(histogramStartDateEdit);
    controlLayout->addLayout(startDateLayout);

    QHBoxLayout *endDateLayout = new QHBoxLayout();
    endDateLayout->addWidget(new QLabel("To:"));
    histogramEndDateEdit = new QDateEdit();
    histogramEndDateEdit->setDate(QDate::currentDate());
    histogramEndDateEdit->setCalendarPopup(true);
    histogramEndDateEdit->setEnabled(false);
    endDateLayout->addWidget(histogramEndDateEdit);
    controlLayout->addLayout(endDateLayout);

    // Symptom selection
    controlLayout->addWidget(new QLabel("Select Symptoms/Metrics:"));

    QHBoxLayout *selectButtonLayout = new QHBoxLayout();
    histogramSelectAllButton = new QPushButton("Select All");
    histogramDeselectAllButton = new QPushButton("Deselect All");
    selectButtonLayout->addWidget(histogramSelectAllButton);
    selectButtonLayout->addWidget(histogramDeselectAllButton);
    controlLayout->addLayout(selectButtonLayout);

    histogramSymptomListWidget = new QListWidget();
    histogramSymptomListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    controlLayout->addWidget(histogramSymptomListWidget);

    // Generate button
    generateHistogramButton = new QPushButton("Generate Stacked Histogram");
    generateHistogramButton->
            setStyleSheet("background-color: #2196F3; color: white; padding: 10px; font-weight: bold;");
    controlLayout->addWidget(generateHistogramButton);

    controlLayout->addStretch();

    // Right panel - plot (using QCustomPlot)
    QVBoxLayout *plotLayout = new QVBoxLayout();

    QLabel *plotTitle = new QLabel("Stacked Histogram Visualization");
    plotTitle->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    plotLayout->addWidget(plotTitle);

    histogramCustomPlot = new QCustomPlot();
    histogramCustomPlot->setOpenGl(true, 4);
    histogramCustomPlot->setMinimumHeight(600);
    plotLayout->addWidget(histogramCustomPlot);

    QLabel *infoLabel = new QLabel(
        "Tip: Stacked histograms show multiple symptoms in separate plots with synchronized X-axes. Use mouse wheel to zoom, drag to pan.");
    infoLabel->setStyleSheet("color: #666; padding: 10px;");
    infoLabel->setWordWrap(true);
    plotLayout->addWidget(infoLabel);

    // Add layouts to main
    mainLayout->addLayout(controlLayout, 1);
    mainLayout->addLayout(plotLayout, 3);

    // Connect signals
    connect(generateHistogramButton, &QPushButton::clicked, this, &MainWindow::loadHistogramData);
    connect(histogramSelectAllButton, &QPushButton::clicked, this, &MainWindow::onHistogramSelectAllSymptoms);
    connect(histogramDeselectAllButton, &QPushButton::clicked, this, &MainWindow::onHistogramDeselectAllSymptoms);
    connect(histogramAllDateRangeCheckbox, &QCheckBox::toggled, [this](bool checked) {
        histogramStartDateEdit->setEnabled(!checked);
        histogramEndDateEdit->setEnabled(!checked);
    });
}

void MainWindow::setupEntryTab() {
    entryTab = new QWidget();
    QHBoxLayout *entryLayout = new QHBoxLayout(entryTab);

    // Left Panel - Notes
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *notesLabel = new QLabel("Sleep Notes:");
    notesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(notesLabel);

    // Date and time inputs
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Date:"));
    dateEdit = new QDateEdit();
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateLayout->addWidget(dateEdit);
    dateLayout->addStretch();
    leftLayout->addLayout(dateLayout);

    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->addWidget(new QLabel("Bedtime:"));
    bedtimeEdit = new QTimeEdit();
    bedtimeEdit->setTime(QTime(22, 0));
    timeLayout->addWidget(bedtimeEdit);

    timeLayout->addWidget(new QLabel("Wake time:"));
    wakeupEdit = new QTimeEdit();
    wakeupEdit->setTime(QTime(7, 0));
    timeLayout->addWidget(wakeupEdit);
    timeLayout->addStretch();
    leftLayout->addLayout(timeLayout);

    notesEdit = new QTextEdit();
    notesEdit->setPlaceholderText("Describe your sleep quality, how you felt, any disturbances, dreams, etc...");
    leftLayout->addWidget(notesEdit);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save Entry");
    saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px; font-weight: bold;");
    clearButton = new QPushButton("Clear");
    clearButton->setStyleSheet("padding: 8px;");

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(clearButton);
    leftLayout->addLayout(buttonLayout);

    // Right Panel - Symptoms
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *symptomsLabel = new QLabel("Symptoms/Factors:");
    symptomsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(symptomsLabel);

    // Scrollable area for symptoms
    symptomsScrollArea = new QScrollArea();
    symptomsScrollArea->setWidgetResizable(true);
    symptomsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    symptomsContainer = new QWidget();
    symptomsLayout = new QVBoxLayout(symptomsContainer);
    symptomsLayout->setAlignment(Qt::AlignTop);

    symptomsScrollArea->setWidget(symptomsContainer);
    rightLayout->addWidget(symptomsScrollArea);

    // Add new symptom button
    addSymptomButton = new QPushButton("Add New Symptom...");
    addSymptomButton->setStyleSheet("padding: 6px;");
    rightLayout->addWidget(addSymptomButton);

    // Add panels to main layout
    entryLayout->addLayout(leftLayout, 2);
    entryLayout->addLayout(rightLayout, 1);

    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveEntry);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearForm);
    connect(addSymptomButton, &QPushButton::clicked, this, &MainWindow::onAddSymptom);
}

void MainWindow::setupHistoryTab() {
    historyTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(historyTab);

    QLabel *titleLabel = new QLabel("Sleep History");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    layout->addWidget(titleLabel);

    // History table
    historyTable = new QTableWidget();
    historyTable->setColumnCount(5);
    historyTable->setHorizontalHeaderLabels({"Date", "Sleep Duration", "Bedtime", "Wake Time", "Symptoms"});
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setAlternatingRowColors(true);
    layout->addWidget(historyTable);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    refreshHistoryButton = new QPushButton("Refresh");
    deleteEntryButton = new QPushButton("Delete Selected");
    deleteEntryButton->setStyleSheet("background-color: #f44336; color: white; padding: 6px;");

    buttonLayout->addWidget(refreshHistoryButton);
    buttonLayout->addWidget(deleteEntryButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(historyTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onHistoryDateSelected);
    connect(refreshHistoryButton, &QPushButton::clicked, this, &MainWindow::loadHistoryData);
    connect(deleteEntryButton, &QPushButton::clicked, this, &MainWindow::onDeleteHistoryEntry);
}

void MainWindow::setupStatisticsTab() {
    statisticsTab = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(statisticsTab);

    // Left panel - controls
    QVBoxLayout *controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel("Plot Configuration");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(titleLabel);

    // Plot type selector
    controlLayout->addWidget(new QLabel("Plot Type:"));
    plotTypeSelector = new QComboBox();
    plotTypeSelector->addItem("Time Series (Line Chart)");
    plotTypeSelector->addItem("Histogram");
    plotTypeSelector->addItem("Correlation View");
    controlLayout->addWidget(plotTypeSelector);

    // Histogram mode selector (only visible for histogram type)
    controlLayout->addWidget(new QLabel("Histogram Mode:"));
    histogramModeSelector = new QComboBox();
    histogramModeSelector->addItem("Overlay (Single Plot)");
    // histogramModeSelector->addItem("Stacked (Multiple Plots)");
    histogramModeSelector->setVisible(false);
    controlLayout->addWidget(histogramModeSelector);

    // Date range
    controlLayout->addWidget(new QLabel("Date Range:"));
    allDateRangeCheckbox = new QCheckBox("Use all data");
    allDateRangeCheckbox->setChecked(true);
    controlLayout->addWidget(allDateRangeCheckbox);

    QHBoxLayout *startDateLayout = new QHBoxLayout();
    startDateLayout->addWidget(new QLabel("From:"));
    startDateEdit = new QDateEdit();
    startDateEdit->setDate(QDate::currentDate().addMonths(-1));
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setEnabled(false);
    startDateLayout->addWidget(startDateEdit);
    controlLayout->addLayout(startDateLayout);

    QHBoxLayout *endDateLayout = new QHBoxLayout();
    endDateLayout->addWidget(new QLabel("To:"));
    endDateEdit = new QDateEdit();
    endDateEdit->setDate(QDate::currentDate());
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setEnabled(false);
    endDateLayout->addWidget(endDateEdit);
    controlLayout->addLayout(endDateLayout);

    // Symptom selection
    controlLayout->addWidget(new QLabel("Select Symptoms/Metrics:"));

    QHBoxLayout *selectButtonLayout = new QHBoxLayout();
    selectAllButton = new QPushButton("Select All");
    deselectAllButton = new QPushButton("Deselect All");
    selectButtonLayout->addWidget(selectAllButton);
    selectButtonLayout->addWidget(deselectAllButton);
    controlLayout->addLayout(selectButtonLayout);

    symptomListWidget = new QListWidget();
    symptomListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    controlLayout->addWidget(symptomListWidget);

    // Generate button
    generatePlotButton = new QPushButton("Generate Plot");
    generatePlotButton->setStyleSheet("background-color: #2196F3; color: white; padding: 10px; font-weight: bold;");
    controlLayout->addWidget(generatePlotButton);

    controlLayout->addStretch();

    // Right panel - plot
    QVBoxLayout *plotLayout = new QVBoxLayout();

    QLabel *plotTitle = new QLabel("Sleep Statistics Visualization");
    plotTitle->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    plotLayout->addWidget(plotTitle);

    customPlot = new QCustomPlot();
    customPlot->setOpenGl(true, 4);
    customPlot->setMinimumHeight(500);
    plotLayout->addWidget(customPlot);

    QLabel *infoLabel = new QLabel("Tip: Select multiple symptoms to compare. Use mouse wheel to zoom, drag to pan.");
    infoLabel->setStyleSheet("color: #666; padding: 10px;");
    infoLabel->setWordWrap(true);
    plotLayout->addWidget(infoLabel);

    // Add layouts to main
    mainLayout->addLayout(controlLayout, 1);
    mainLayout->addLayout(plotLayout, 3);

    // Connect signals
    connect(generatePlotButton, &QPushButton::clicked, this, &MainWindow::loadStatisticsData);
    connect(plotTypeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onPlotTypeChanged);
    connect(selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllSymptoms);
    connect(deselectAllButton, &QPushButton::clicked, this, &MainWindow::onDeselectAllSymptoms);
    connect(allDateRangeCheckbox, &QCheckBox::toggled, [this](bool checked) {
        startDateEdit->setEnabled(!checked);
        endDateEdit->setEnabled(!checked);
    });
}

void MainWindow::createUserToolbar() {
    userToolbar = new QToolBar("User", this);
    userToolbar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, userToolbar);

    // User label
    userLabel = new QLabel();
    userLabel->setStyleSheet("padding: 5px 10px; font-weight: bold;");
    userToolbar->addWidget(userLabel);

    userToolbar->addSeparator();

    // Logout button
    logoutButton = new QPushButton("Logout");
    logoutButton->setStyleSheet("padding: 5px 15px;");
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::onLogout);
    userToolbar->addWidget(logoutButton);

    // Connect to user manager
    connect(&UserManager::instance(), &UserManager::userLoggedIn,
            this, &MainWindow::onUserChanged);
    connect(&UserManager::instance(), &UserManager::userLoggedOut,
            this, &MainWindow::onUserChanged);

    onUserChanged();
}

void MainWindow::updateWindowTitle() {
    QString title = "Sleep Quality Tracker";
    if (UserManager::instance().isLoggedIn()) {
        title += QString(" - %1").arg(UserManager::instance().getCurrentUsername());
    }
    setWindowTitle(title);
}

void MainWindow::onUserChanged() {
    bool loggedIn = UserManager::instance().isLoggedIn();

    if (loggedIn) {
        User *user = UserManager::instance().getCurrentUser();
        QString displayText = user->getDisplayName().isEmpty() ? user->getUsername() : user->getDisplayName();
        userLabel->setText(QString("User: %1").arg(displayText));
        logoutButton->setEnabled(true);
    } else {
        userLabel->setText("Not logged in");
        logoutButton->setEnabled(false);
    }

    updateWindowTitle();
}

void MainWindow::onLogout() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Save current data before logging out
        if (UserManager::instance().isLoggedIn()) {
            saveSymptoms();
        }

        UserManager::instance().logout();

        // Show login dialog again
        LoginDialog loginDialog(this);
        if (loginDialog.exec() == QDialog::Accepted) {
            // User logged in successfully, reload data
            loadSymptoms();
            rebuildSymptomWidgets();
            onClearForm();
            updateWindowTitle();
        } else {
            // User cancelled login, close the application
            close();
        }
    }
}

void MainWindow::onTabChanged(int index) {
    if (index == 1) {
        // History tab
        loadHistoryData();
    } else if (index == 2) {
        // Statistics tab
        loadStatisticsData();
    } else if (index == 3) {
        // Histograms tab
        loadHistogramData();
    }
}

QList<QVariantMap> MainWindow::loadAllEntries() {
    QList<QVariantMap> allEntries;

    if (!UserManager::instance().isLoggedIn()) {
        return allEntries;
    }

    QString symptomFile = getSymptomDataFile();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    QByteArray data = DataEncryption::loadEncrypted(symptomFile, password);

    if (!data.isEmpty()) {
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);
        in >> allEntries;
    }

    return allEntries;
}

void MainWindow::loadHistoryData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    historyTable->setRowCount(0);

    QList<QVariantMap> entries = loadAllEntries();

    // Sort by date descending
    std::sort(entries.begin(), entries.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["date"].toDate() > b["date"].toDate();
    });

    historyTable->setRowCount(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        const QVariantMap &entry = entries[i];

        // Date
        QTableWidgetItem *dateItem = new QTableWidgetItem(entry["date"].toDate().toString("yyyy-MM-dd"));
        historyTable->setItem(i, 0, dateItem);

        // Sleep duration
        double duration = entry["sleep_duration"].toDouble();
        QTableWidgetItem *durationItem = new QTableWidgetItem(QString::number(duration, 'f', 1) + " hrs");
        historyTable->setItem(i, 1, durationItem);

        // For bedtime and wake time, we need to load from the daily file
        QString dataDir = getCurrentDataDirectory();
        QString filename = QString("%1/sleep_%2.dat")
                .arg(dataDir)
                .arg(entry["date"].toDate().toString("yyyy-MM-dd"));

        QString bedtimeStr = "-";
        QString waketimeStr = "-";

        QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();
        QByteArray dailyData = DataEncryption::loadEncrypted(filename, password);

        if (!dailyData.isEmpty()) {
            QDataStream in(&dailyData, QIODevice::ReadOnly);
            in.setVersion(QDataStream::Qt_5_15);

            QDateTime timestamp;
            QDate date;
            QTime bedtime, waketime;

            in >> timestamp >> date >> bedtime >> waketime;

            bedtimeStr = bedtime.toString("HH:mm");
            waketimeStr = waketime.toString("HH:mm");
        }

        historyTable->setItem(i, 2, new QTableWidgetItem(bedtimeStr));
        historyTable->setItem(i, 3, new QTableWidgetItem(waketimeStr));

        // Symptoms
        QStringList symptomList;
        for (const Symptom &s: symptoms) {
            double value = entry.value(s.getName(), 0.0).toDouble();
            if (value > 0) {
                if (s.getType() == SymptomType::Binary) {
                    symptomList << s.getName();
                } else {
                    symptomList << QString("%1 (%2)").arg(s.getName()).arg(value);
                }
            }
        }

        QTableWidgetItem *symptomsItem = new QTableWidgetItem(symptomList.join(", "));
        historyTable->setItem(i, 4, symptomsItem);
    }

    historyTable->resizeColumnsToContents();
}

void MainWindow::onHistoryDateSelected(int row, int column) {
    Q_UNUSED(column);

    QTableWidgetItem *dateItem = historyTable->item(row, 0);
    if (!dateItem) return;

    QDate selectedDate = QDate::fromString(dateItem->text(), "yyyy-MM-dd");

    // Load the full entry for this date
    QString dataDir = getCurrentDataDirectory();
    QString filename = QString("%1/sleep_%2.dat")
            .arg(dataDir)
            .arg(selectedDate.toString("yyyy-MM-dd"));

    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();
    QByteArray data = DataEncryption::loadEncrypted(filename, password);

    if (!data.isEmpty()) {
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);

        QDateTime timestamp;
        QDate date;
        QTime bedtime, waketime;
        double hours;
        QString notes;
        QList<QPair<QString, double> > symptomData;

        in >> timestamp >> date >> bedtime >> waketime >> hours >> notes >> symptomData;

        // Show details dialog
        QDialog dialog(this);
        dialog.setWindowTitle(QString("Sleep Entry - %1").arg(date.toString("yyyy-MM-dd")));
        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        layout->addWidget(new QLabel(QString("<b>Date:</b> %1").arg(date.toString("yyyy-MM-dd"))));
        layout->addWidget(new QLabel(QString("<b>Bedtime:</b> %1").arg(bedtime.toString("HH:mm"))));
        layout->addWidget(new QLabel(QString("<b>Wake time:</b> %1").arg(waketime.toString("HH:mm"))));
        layout->addWidget(new QLabel(QString("<b>Sleep duration:</b> %1 hours").arg(hours, 0, 'f', 1)));

        layout->addWidget(new QLabel("<b>Notes:</b>"));
        QTextEdit *notesDisplay = new QTextEdit();
        notesDisplay->setPlainText(notes);
        notesDisplay->setReadOnly(true);
        notesDisplay->setMaximumHeight(150);
        layout->addWidget(notesDisplay);

        layout->addWidget(new QLabel("<b>Symptoms:</b>"));
        QTextEdit *symptomsDisplay = new QTextEdit();
        QStringList symptomStrings;
        for (const auto &pair: symptomData) {
            symptomStrings << QString("%1: %2").arg(pair.first).arg(pair.second);
        }
        symptomsDisplay->setPlainText(symptomStrings.join("\n"));
        symptomsDisplay->setReadOnly(true);
        symptomsDisplay->setMaximumHeight(150);
        layout->addWidget(symptomsDisplay);

        QPushButton *closeButton = new QPushButton("Close");
        connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        layout->addWidget(closeButton);

        dialog.exec();
    }
}

void MainWindow::onDeleteHistoryEntry() {
    int currentRow = historyTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "No Selection", "Please select an entry to delete.");
        return;
    }

    QTableWidgetItem *dateItem = historyTable->item(currentRow, 0);
    if (!dateItem) return;

    QDate selectedDate = QDate::fromString(dateItem->text(), "yyyy-MM-dd");

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Entry",
        QString("Are you sure you want to delete the entry for %1?").arg(selectedDate.toString("yyyy-MM-dd")),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Delete the daily file
        QString dataDir = getCurrentDataDirectory();
        QString filename = QString("%1/sleep_%2.dat")
                .arg(dataDir)
                .arg(selectedDate.toString("yyyy-MM-dd"));

        QFile::remove(filename);

        // Remove from summary file (C++20 compatible way)
        QList<QVariantMap> entries = loadAllEntries();
        QList<QVariantMap> filteredEntries;
        for (const auto &entry: entries) {
            if (entry["date"].toDate() != selectedDate) {
                filteredEntries.append(entry);
            }
        }

        // Save updated summary
        QString symptomFile = getSymptomDataFile();
        QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_15);
        out << filteredEntries;

        DataEncryption::saveEncrypted(symptomFile, data, password);

        // Reload table
        loadHistoryData();

        QMessageBox::information(this, "Deleted", "Entry deleted successfully.");
    }
}

void MainWindow::loadStatisticsData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    // Update symptom list if empty
    if (symptomListWidget->count() == 0) {
        QListWidgetItem *sleepItem = new QListWidgetItem("Sleep Duration");
        sleepItem->setFlags(sleepItem->flags() | Qt::ItemIsUserCheckable);
        sleepItem->setCheckState(Qt::Checked);
        symptomListWidget->addItem(sleepItem);

        for (const Symptom &s: symptoms) {
            QListWidgetItem *item = new QListWidgetItem(s.getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            symptomListWidget->addItem(item);
        }
    }

    // Get selected symptoms
    QStringList selectedSymptoms;
    for (int i = 0; i < symptomListWidget->count(); ++i) {
        QListWidgetItem *item = symptomListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedSymptoms.append(item->text());
        }
    }

    if (selectedSymptoms.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select at least one symptom or metric to plot.");
        return;
    }

    // Load all entries
    QList<QVariantMap> entries = loadAllEntries();

    if (entries.isEmpty()) {
        customPlot->clearGraphs();
        customPlot->clearPlottables();
        customPlot->replot();
        QMessageBox::information(this, "No Data", "No sleep data available for analysis.");
        return;
    }

    // Filter by date range if needed
    if (!allDateRangeCheckbox->isChecked()) {
        entries = filterEntriesByDateRange(entries, startDateEdit->date(), endDateEdit->date());
        if (entries.isEmpty()) {
            QMessageBox::information(this, "No Data", "No data in selected date range.");
            return;
        }
    }

    // Sort by date
    std::sort(entries.begin(), entries.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["date"].toDate() < b["date"].toDate();
    });

    // Plot based on selected type
    int plotType = plotTypeSelector->currentIndex();

    switch (plotType) {
        case 0: // Time Series
            plotTimeSeriesData(entries, selectedSymptoms);
            break;
        case 1: // Histogram
            plotHistogramData(entries, selectedSymptoms);
            break;
        case 2: // Correlation
            if (selectedSymptoms.size() < 2) {
                QMessageBox::warning(this, "Multiple Selection Required",
                                     "Correlation view requires at least 2 symptoms selected.");
                return;
            }
            plotCorrelationData(entries, selectedSymptoms);
            break;
    }
}

QList<QVariantMap> MainWindow::filterEntriesByDateRange(const QList<QVariantMap> &entries, const QDate &start,
                                                        const QDate &end) {
    QList<QVariantMap> filtered;
    for (const auto &entry: entries) {
        QDate entryDate = entry["date"].toDate();
        if (entryDate >= start && entryDate <= end) {
            filtered.append(entry);
        }
    }
    return filtered;
}

void MainWindow::plotHistogramData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    if (selectedSymptoms.isEmpty()) {
        return;
    }

    int histogramMode = histogramModeSelector->currentIndex();

    if (histogramMode == 0) {
        // Overlay mode
        plotHistogramOverlay(entries, selectedSymptoms);
    } else {
        // Stacked mode
        plotHistogramStacked(entries, selectedSymptoms);
    }
}

void MainWindow::plotTimeSeriesData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    customPlot->clearGraphs();
    customPlot->clearPlottables();

    QVector<double> xData;
    QVector<QString> dateLabels;

    for (int i = 0; i < entries.size(); ++i) {
        xData.append(i);
        dateLabels.append(entries[i]["date"].toDate().toString("MM/dd"));
    }

    // Color palette for multiple lines
    QVector<QColor> colors = {
        QColor(33, 150, 243), // Blue
        QColor(76, 175, 80), // Green
        QColor(255, 152, 0), // Orange
        QColor(156, 39, 176), // Purple
        QColor(244, 67, 54), // Red
        QColor(0, 188, 212), // Cyan
        QColor(255, 235, 59), // Yellow
        QColor(121, 85, 72) // Brown
    };

    int colorIndex = 0;

    for (const QString &symptomName: selectedSymptoms) {
        QVector<double> yData;

        for (const auto &entry: entries) {
            if (symptomName == "Sleep Duration") {
                yData.append(entry["sleep_duration"].toDouble());
            } else {
                yData.append(entry.value(symptomName, 0.0).toDouble());
            }
        }

        customPlot->addGraph();
        customPlot->graph()->setData(xData, yData);
        customPlot->graph()->setName(symptomName);

        QColor color = colors[colorIndex % colors.size()];
        customPlot->graph()->setPen(QPen(color, 2));
        customPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color, color, 5));

        colorIndex++;
    }

    customPlot->xAxis->setLabel("Date");
    customPlot->yAxis->setLabel("Value");

    // Add buffer space on both ends (5% of range on each side)
    double xBuffer = entries.size() * 0.05;
    customPlot->xAxis->setRange(-xBuffer, entries.size() - 1 + xBuffer);
    customPlot->rescaleAxes();

    // Add some padding to y-axis as well (10% on top)
    QCPRange yRange = customPlot->yAxis->range();
    double yPadding = yRange.size() * 0.1;
    customPlot->yAxis->setRange(yRange.lower - yPadding * 0.5, yRange.upper + yPadding);

    // Set custom tick labels
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    for (int i = 0; i < dateLabels.size(); ++i) {
        textTicker->addTick(i, dateLabels[i]);
    }
    customPlot->xAxis->setTicker(textTicker);
    customPlot->xAxis->setTickLabelRotation(45);

    customPlot->legend->setVisible(true);
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    customPlot->replot();
}

void MainWindow::plotHistogramOverlay(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    // --- SAFE CLEANUP BEFORE REBUILD ---
    for (QPointer<QCPAxis> xAxis: qAsConst(synchronizedXAxes)) {
        if (xAxis)
            QObject::disconnect(xAxis, nullptr, this, nullptr);
    }
    synchronizedXAxes.clear();

    customPlot->clearGraphs();
    customPlot->clearPlottables();

    if (customPlot->plotLayout()->elementCount() == 0) {
        customPlot->plotLayout()->simplify(); // Reset to default layout
        QCPAxisRect *axisRect = new QCPAxisRect(customPlot);
        customPlot->plotLayout()->addElement(0, 0, axisRect);

        // Reassign axis pointers to the new axis rect
        customPlot->setCurrentLayer("main");
    }

    // Color palette
    QVector<QColor> colors = {
        QColor(33, 150, 243, 150), QColor(76, 175, 80, 150),
        QColor(255, 152, 0, 150), QColor(156, 39, 176, 150),
        QColor(244, 67, 54, 150), QColor(0, 188, 212, 150)
    };

    // Organize data by date
    QMap<QDate, QMap<QString, double> > dateData;
    for (const auto &entry: entries) {
        QDate date = QDate::fromString(entry["date"].toString(), Qt::ISODate);
        if (!date.isValid()) continue;

        for (const QString &symptom: selectedSymptoms)
            dateData[date].insert(symptom, 0.0);

        for (const QString &symptomName: selectedSymptoms) {
            if (symptomName == "Sleep Duration") {
                dateData[date][symptomName] = entry["sleep_duration"].toDouble();
            } else if (entry.contains(symptomName)) {
                QVariant value = entry[symptomName];

                // Handle numeric, boolean, and other types
                if (value.type() == QVariant::Bool) {
                    if (value.toBool())
                        dateData[date][symptomName] += 1.0;
                } else if (value.canConvert(QMetaType::Double)) {
                    dateData[date][symptomName] += value.toDouble();
                }
            }
        }
    }

    QList<QDate> dates = dateData.keys();
    std::sort(dates.begin(), dates.end());
    if (dates.isEmpty()) {
        customPlot->replot();
        return;
    }

    QVector<double> dateNumbers;
    for (const QDate &date: dates)
        dateNumbers.append(QDateTime(date).toMSecsSinceEpoch() / 1000.0);

    double totalSeconds = dateNumbers.last() - dateNumbers.first();
    double barWidth = totalSeconds / (dates.size() * selectedSymptoms.size() * 1.5);
    double maxValue = 0;

    for (int i = 0; i < selectedSymptoms.size(); ++i) {
        const QString &symptom = selectedSymptoms[i];
        QVector<double> values;
        for (const QDate &date: dates) {
            double val = dateData[date].value(symptom, 0.0);
            values.append(val);
            maxValue = std::max(maxValue, val);
        }

        QCPBars *bars = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        QVector<double> offsetDates;
        double offset = (i - (selectedSymptoms.size() - 1) / 2.0) * barWidth * 0.2;
        for (double d: dateNumbers)
            offsetDates.append(d + offset);

        bars->setData(offsetDates, values);
        QColor color = colors[i % colors.size()];
        bars->setPen(QPen(color.darker(), 1));
        bars->setBrush(color);
        bars->setWidth(barWidth);
        bars->setName(symptom);
    }

    customPlot->xAxis->setLabel("Date");
    customPlot->yAxis->setLabel(selectedSymptoms.contains("Sleep Duration")
                                    ? "Value (Count/Hours)"
                                    : "Symptom Count");

    double xBuffer = (dateNumbers.last() - dateNumbers.first()) * 0.5;
    customPlot->xAxis->setRange(dateNumbers.first() - xBuffer, dateNumbers.last() + xBuffer);
    customPlot->yAxis->setRange(0, maxValue * 1.2);

    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("MMM d\nyyyy");
    customPlot->xAxis->setTicker(dateTicker);

    customPlot->legend->setVisible(true);
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    customPlot->replot();
}

// void MainWindow::plotHistogramStacked(const QList<QVariantMap>& entries, const QStringList& selectedSymptoms) {
//     //qcustomplot
//     for (QPointer<QCPAxis> xAxis : qAsConst(synchronizedXAxes)) {
//         if (xAxis) {
//             // Disconnect ALL signals from this axis to ALL receivers
//             QObject::disconnect(xAxis, nullptr, this, nullptr);
//         }
//     }
//     synchronizedXAxes.clear(); // Clear the list of potentially dangling pointers
//
//     // Also disconnect the syncXAxes slot from ANY sender that might still be connected
//     // NOTE: This uses the old-style disconnect since the signature is ambiguous across C++ versions.
//     disconnect(this, SLOT(syncXAxes(QCPRange)));
//
//     customPlot->setNoAntialiasingOnDrag(true);
//     customPlot->clearGraphs();
//     customPlot->clearPlottables();
//
//     // Now safe to clear layout since all signal connections are gone
//     customPlot->plotLayout()->clear();
//
//     QMap<QDate, QMap<QString, double>> dateData;
//
//     for (const auto& entry : entries) {
//         QDate date = entry["date"].toDate();
//         if (!date.isValid()) continue;
//
//         auto& dateMap = dateData[date];
//         for (const QString& s : selectedSymptoms)
//             if (!dateMap.contains(s))
//                 dateMap[s] = 0.0;
//
//         for (const QString& s : selectedSymptoms) {
//             if (s == "Sleep Duration") {
//                 dateMap[s] = entry["sleep_duration"].toDouble();
//             } else if (entry.contains(s)) {
//                 QVariant value = entry[s];
//                 if (value.type() == QVariant::Bool) {
//                     if (value.toBool()) dateMap[s] += 1.0;
//                 } else dateMap[s] += value.toDouble();
//             }
//         }
//     }
//
//     QList<QDate> dates = dateData.keys();
//     std::sort(dates.begin(), dates.end());
//     if (dates.isEmpty()) {
//         customPlot->replot();
//         return;
//     }
//
//     QVector<double> dateNumbers;
//     for (const QDate& date : dates)
//         dateNumbers.append(date.startOfDay().toMSecsSinceEpoch() / 1000.0);
//
//     double minDate = dateNumbers.first();
//     double maxDate = dateNumbers.last();
//     double xBuffer = (maxDate - minDate) * 0.5;
//     double barWidth = (maxDate - minDate) / (dates.size() * 1.2);
//
//     QVector<QVector<double>> symptomValues(selectedSymptoms.size());
//     QVector<double> maxY(selectedSymptoms.size(), 0.0);
//
//     for (int i = 0; i < selectedSymptoms.size(); ++i) {
//         const QString& name = selectedSymptoms[i];
//         QVector<double>& vals = symptomValues[i];
//         for (const QDate& date : dates) {
//             double v = dateData[date].value(name, 0.0);
//             vals.append(v);
//             maxY[i] = std::max(maxY[i], v);
//         }
//     }
//
//     int numPlots = selectedSymptoms.size();
//     synchronizedXAxes.clear();
//     synchronizedXAxes.reserve(numPlots);
//
//     // Color palette
//     QVector<QColor> colors = {
//         QColor(33, 150, 243, 150), QColor(76, 175, 80, 150),
//         QColor(255, 152, 0, 150), QColor(156, 39, 176, 150),
//         QColor(244, 67, 54, 150), QColor(0, 188, 212, 150)
//     };
//
//     for (int i = 0; i < numPlots; ++i) {
//         QCPAxisRect* axisRect = new QCPAxisRect(customPlot);
//         customPlot->plotLayout()->addElement(i, 0, axisRect);
//
//         QCPAxis* xAxis = axisRect->axis(QCPAxis::atBottom);
//         QCPAxis* yAxis = axisRect->axis(QCPAxis::atLeft);
//         synchronizedXAxes.append(xAxis); // Store the axis pointer
//
//         QCPBars* bars = new QCPBars(xAxis, yAxis);
//         bars->setData(dateNumbers, symptomValues[i]);
//         QColor color = colors[i % colors.size()];
//         bars->setPen(QPen(color));
//         bars->setBrush(QBrush(color));
//         bars->setWidth(barWidth);
//
//         if (i == numPlots - 1) {
//             QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
//             ticker->setDateTimeFormat("MMM d");
//             xAxis->setTicker(ticker);
//             xAxis->setLabel("Date");
//         } else {
//             xAxis->setTickLabels(false);
//         }
//
//         yAxis->setLabel(selectedSymptoms[i] == "Sleep Duration" ? "Hours" : "Count");
//         yAxis->setRange(0, maxY[i] * 1.1);
//         xAxis->setRange(minDate - xBuffer, maxDate + xBuffer);
//
//         QCPTextElement* title = new QCPTextElement(customPlot, selectedSymptoms[i]);
//         axisRect->insetLayout()->addElement(title, Qt::AlignTop | Qt::AlignHCenter);
//     }
//
//     // Connect synchronization AFTER all axes are created and added
//     // --- CRITICAL FIX: Use the argument-taking syncXAxes slot ---
//     for (int i = 0; i < synchronizedXAxes.size(); ++i) {
//         QPointer<QCPAxis> xAxis = synchronizedXAxes[i];
//         if (xAxis) {
//             // Connect to the argument-taking slot, which is safe from recursion
//             connect(xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged),
//                     this, &MainWindow::syncXAxes, Qt::UniqueConnection);
//         }
//     }
//     // --- END CRITICAL FIX ---
//
//     customPlot->setNotAntialiasedElements(QCP::aeNone);
//     customPlot->replot();
// }

// In your MainWindow constructor or initialization:
void MainWindow::initializeHistogram() {
    // Enable mouse tracking
    histogramCustomPlot->setMouseTracking(true);

    // Configure zooming behavior
    histogramCustomPlot->axisRect()->setRangeZoomFactor(0.85); // Slower zoom
    histogramCustomPlot->axisRect()->setRangeDrag(0); // Start with no drag

    // Add zoom instructions to tooltip
    histogramCustomPlot->setToolTip("Scroll: Vertical zoom\nDrag: Horizontal scroll\nRight-click: Context menu");
}

void MainWindow::showHistogramContextMenu(const QPoint &pos) {
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *resetZoomAction = menu->addAction("Reset Zoom");
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::resetHistogramZoom);

    menu->popup(histogramCustomPlot->mapToGlobal(pos));
}

void MainWindow::resetHistogramZoom() {
    // Replot with original ranges
    // You might want to store the original date ranges and replot
    histogramCustomPlot->rescaleAxes();
    histogramCustomPlot->replot();
}

void MainWindow::plotHistogramStacked(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    // Clear previous synchronization
    for (QPointer<QCPAxis> xAxis: qAsConst(synchronizedXAxes)) {
        if (xAxis) {
            QObject::disconnect(xAxis, nullptr, this, nullptr);
        }
    }
    synchronizedXAxes.clear();
    disconnect(this, SLOT(syncXAxes(QCPRange)));

    // Use histogramCustomPlot instead of customPlot
    histogramCustomPlot->setNoAntialiasingOnDrag(true);
    histogramCustomPlot->clearGraphs();
    histogramCustomPlot->clearPlottables();
    histogramCustomPlot->plotLayout()->clear();

    // Enable interactions for scrolling and zooming
    histogramCustomPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    QMap<QDate, QMap<QString, double> > dateData;

    for (const auto &entry: entries) {
        QDate date = entry["date"].toDate();
        if (!date.isValid()) continue;

        auto &dateMap = dateData[date];
        for (const QString &s: selectedSymptoms)
            if (!dateMap.contains(s))
                dateMap[s] = 0.0;

        for (const QString &s: selectedSymptoms) {
            if (s == "Sleep Duration") {
                dateMap[s] = entry["sleep_duration"].toDouble();
            } else if (entry.contains(s)) {
                QVariant value = entry[s];
                if (value.type() == QVariant::Bool) {
                    if (value.toBool()) dateMap[s] += 1.0;
                } else dateMap[s] += value.toDouble();
            }
        }
    }

    QList<QDate> dates = dateData.keys();
    std::sort(dates.begin(), dates.end());
    if (dates.isEmpty()) {
        histogramCustomPlot->replot();
        return;
    }

    QVector<double> dateNumbers;
    for (const QDate &date: dates)
        dateNumbers.append(date.startOfDay().toMSecsSinceEpoch() / 1000.0);

    double minDate = dateNumbers.first();
    double maxDate = dateNumbers.last();
    double xBuffer = (maxDate - minDate) * 0.1; // Reduced buffer for better scrolling
    double barWidth = (maxDate - minDate) / (dates.size() * 1.2);

    QVector<QVector<double> > symptomValues(selectedSymptoms.size());
    QVector<double> maxY(selectedSymptoms.size(), 0.0);

    for (int i = 0; i < selectedSymptoms.size(); ++i) {
        const QString &name = selectedSymptoms[i];
        QVector<double> &vals = symptomValues[i];
        for (const QDate &date: dates) {
            double v = dateData[date].value(name, 0.0);
            vals.append(v);
            maxY[i] = std::max(maxY[i], v);
        }
    }

    int numPlots = selectedSymptoms.size();
    synchronizedXAxes.clear();
    synchronizedXAxes.reserve(numPlots);

    // Color palette
    QVector<QColor> colors = {
        QColor(33, 150, 243, 150), QColor(76, 175, 80, 150),
        QColor(255, 152, 0, 150), QColor(156, 39, 176, 150),
        QColor(244, 67, 54, 150), QColor(0, 188, 212, 150)
    };

    for (int i = 0; i < numPlots; ++i) {
        QCPAxisRect *axisRect = new QCPAxisRect(histogramCustomPlot);
        histogramCustomPlot->plotLayout()->addElement(i, 0, axisRect);

        // Configure axis rect for zooming and scrolling
        axisRect->setRangeDrag(Qt::Horizontal);  // Allow horizontal dragging (scrolling)
        axisRect->setRangeZoom(Qt::Vertical);    // Allow vertical zooming
        axisRect->setRangeZoomAxes(axisRect->axis(QCPAxis::atBottom), axisRect->axis(QCPAxis::atLeft));

        QCPAxis *xAxis = axisRect->axis(QCPAxis::atBottom);
        QCPAxis *yAxis = axisRect->axis(QCPAxis::atLeft);
        synchronizedXAxes.append(xAxis);

        QCPBars *bars = new QCPBars(xAxis, yAxis);
        bars->setData(dateNumbers, symptomValues[i]);
        QColor color = colors[i % colors.size()];
        bars->setPen(QPen(color));
        bars->setBrush(QBrush(color));
        bars->setWidth(barWidth);

        // Set up date formatting for x-axis
        QSharedPointer<QCPAxisTickerDateTime> ticker(new QCPAxisTickerDateTime);
        ticker->setDateTimeFormat("MMM d");

        if (i == numPlots - 1) {
            xAxis->setTicker(ticker);
            xAxis->setLabel("Date");
        } else {
            xAxis->setTicker(ticker);
            xAxis->setTickLabels(false); // Only show labels on bottom plot
        }

        yAxis->setLabel(selectedSymptoms[i] == "Sleep Duration" ? "Hours" : "Count");
        yAxis->setRange(0, maxY[i] * 1.1);

        // Set initial X range with some padding
        xAxis->setRange(minDate - xBuffer, maxDate + xBuffer);

        QCPTextElement *title = new QCPTextElement(histogramCustomPlot, selectedSymptoms[i]);
        axisRect->insetLayout()->addElement(title, Qt::AlignTop | Qt::AlignHCenter);

        // Add margin to prevent title overlap
        axisRect->setAutoMargins(QCP::msLeft | QCP::msRight | QCP::msBottom);
        axisRect->setMargins(QMargins(50, 40, 50, 30));
    }

    // Connect synchronization for all x-axes
    for (int i = 0; i < synchronizedXAxes.size(); ++i) {
        QPointer<QCPAxis> xAxis = synchronizedXAxes[i];
        if (xAxis) {
            connect(xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                    this, &MainWindow::syncXAxes, Qt::UniqueConnection);
        }
    }

    // Add context menu for resetting zoom
    histogramCustomPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(histogramCustomPlot, &QCustomPlot::customContextMenuRequested,
            this, &MainWindow::showHistogramContextMenu);

    histogramCustomPlot->setNotAntialiasedElements(QCP::aeNone);
    histogramCustomPlot->replot();
}

void MainWindow::syncXAxes(const QCPRange &newRange) {
    QCPAxis *senderAxis = qobject_cast<QCPAxis *>(sender());
    if (!senderAxis || !customPlot || !customPlot->plotLayout())
        return;

    // Use the range provided by the signal, not by calling senderAxis->range()
    // This removes the need for the static QCPRange lastRange check.

    // CRITICAL: Block signals on the *sender* to prevent immediate re-entry
    // from the replot when other axes update.
    bool oldSenderBlock = senderAxis->blockSignals(true);

    // Block signals on the entire widget temporarily to prevent all related signals
    // from firing until all ranges are set.
    bool oldPlotBlock = customPlot->blockSignals(true);

    // 1. Iterate over the synchronized axes
    for (QPointer<QCPAxis> targetAxis: qAsConst(synchronizedXAxes)) {
        // 2. CRITICAL CHECK: Ensure the axis is still alive and is not the sender
        if (targetAxis && targetAxis.data() != senderAxis) {
            // 3. Set the range only if it's different
            if (targetAxis->range() != newRange) {
                // We don't need to block signals on targetAxis here because
                // we blocked signals on the whole customPlot widget.
                targetAxis->setRange(newRange);
            }
        }
    }

    // Unblock signals in reverse order
    customPlot->blockSignals(oldPlotBlock);
    senderAxis->blockSignals(oldSenderBlock);

    // Replot once at the end
    customPlot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::plotCorrelationData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    customPlot->clearGraphs();
    customPlot->clearPlottables();

    // For correlation view, create scatter plots between pairs
    // We'll plot the first symptom vs each other symptom
    QString baseSymptom = selectedSymptoms.first();

    QVector<QColor> colors = {
        QColor(76, 175, 80), // Green
        QColor(255, 152, 0), // Orange
        QColor(156, 39, 176), // Purple
        QColor(244, 67, 54), // Red
        QColor(0, 188, 212), // Cyan
    };

    for (int i = 1; i < selectedSymptoms.size(); ++i) {
        QString compareSymptom = selectedSymptoms[i];

        QVector<double> xData, yData;

        for (const auto &entry: entries) {
            double xVal = (baseSymptom == "Sleep Duration")
                              ? entry["sleep_duration"].toDouble()
                              : entry.value(baseSymptom, 0.0).toDouble();
            double yVal = (compareSymptom == "Sleep Duration")
                              ? entry["sleep_duration"].toDouble()
                              : entry.value(compareSymptom, 0.0).toDouble();

            xData.append(xVal);
            yData.append(yVal);
        }

        customPlot->addGraph();
        customPlot->graph()->setData(xData, yData);
        customPlot->graph()->setName(QString("%1 vs %2").arg(baseSymptom).arg(compareSymptom));
        customPlot->graph()->setLineStyle(QCPGraph::lsNone);

        QColor color = colors[(i - 1) % colors.size()];
        customPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color, color, 7));
    }

    customPlot->xAxis->setLabel(baseSymptom);
    customPlot->yAxis->setLabel("Compared Symptoms");
    customPlot->rescaleAxes();

    // Add buffer padding (10% on all sides)
    QCPRange xRange = customPlot->xAxis->range();
    QCPRange yRange = customPlot->yAxis->range();
    double xPadding = xRange.size() * 0.1;
    double yPadding = yRange.size() * 0.1;

    customPlot->xAxis->setRange(xRange.lower - xPadding, xRange.upper + xPadding);
    customPlot->yAxis->setRange(yRange.lower - yPadding, yRange.upper + yPadding);

    customPlot->legend->setVisible(true);
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    customPlot->replot();
}

// void MainWindow::onPlotTypeChanged(int index) {
//     // Show/hide histogram mode selector based on plot type
//     bool isHistogram = (index == 1);
//     histogramModeSelector->setVisible(isHistogram);
//
//     // Find the label before the histogram mode selector and show/hide it too
//     QLabel* histogramLabel = nullptr;
//     QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(histogramModeSelector->parentWidget()->layout());
//     if (layout) {
//         int idx = layout->indexOf(histogramModeSelector);
//         if (idx > 0) {
//             QLayoutItem* item = layout->itemAt(idx - 1);
//             if (item && item->widget()) {
//                 histogramLabel = qobject_cast<QLabel*>(item->widget());
//                 if (histogramLabel && histogramLabel->text() == "Histogram Mode:") {
//                     histogramLabel->setVisible(isHistogram);
//                 }
//             }
//         }
//     }
// }
void MainWindow::onPlotTypeChanged(int index) {
    // Only show histogram mode selector if histogram is selected in statistics tab
    // Since we moved stacked histograms to their own tab, we can remove this logic
    // or keep only overlay histogram in statistics tab
    bool isHistogram = (index == 1);
}

void MainWindow::onSelectAllSymptoms() {
    for (int i = 0; i < symptomListWidget->count(); ++i) {
        symptomListWidget->item(i)->setCheckState(Qt::Checked);
    }
}

void MainWindow::onDeselectAllSymptoms() {
    for (int i = 0; i < symptomListWidget->count(); ++i) {
        symptomListWidget->item(i)->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::loadSymptoms() {
    symptoms.clear();

    if (!UserManager::instance().isLoggedIn()) {
        return; // Don't load if not logged in
    }

    // Load from user-specific encrypted binary file
    QString userSymptomsFile = getCurrentDataDirectory() + "/symptoms.dat";
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    QByteArray data = DataEncryption::loadEncrypted(userSymptomsFile, password);

    if (!data.isEmpty()) {
        // Deserialize from binary
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);

        qint32 count;
        in >> count;

        for (qint32 i = 0; i < count; ++i) {
            QString name, unit, typeStr;
            in >> name >> typeStr >> unit;

            SymptomType type = Symptom::stringToType(typeStr);
            symptoms.append(Symptom(name, type, unit));
        }
    }

    // Add defaults if empty
    if (symptoms.isEmpty()) {
        symptoms.append(Symptom("Insomnia", SymptomType::Binary));
        symptoms.append(Symptom("Snoring", SymptomType::Binary));
        symptoms.append(Symptom("Nightmares", SymptomType::Binary));
        symptoms.append(Symptom("Restless", SymptomType::Binary));
        symptoms.append(Symptom("Tired upon waking", SymptomType::Binary));
        symptoms.append(Symptom("Difficulty falling asleep", SymptomType::Binary));
        symptoms.append(Symptom("Woke up during night", SymptomType::Count, "times"));
        symptoms.append(Symptom("Sleep apnea symptoms", SymptomType::Binary));
        symptoms.append(Symptom("Stress/Anxiety", SymptomType::Binary));
        symptoms.append(Symptom("Caffeine before bed", SymptomType::Quantity, "mg"));
        symptoms.append(Symptom("Alcohol consumption", SymptomType::Quantity, "drinks"));
        symptoms.append(Symptom("Exercise during day", SymptomType::Binary));
        symptoms.append(Symptom("Screen time before bed", SymptomType::Quantity, "hours"));
        symptoms.append(Symptom("Room too hot/cold", SymptomType::Binary));
    }
}

void MainWindow::saveSymptoms() {
    if (!UserManager::instance().isLoggedIn()) {
        return; // Don't save if not logged in
    }

    // Save to user-specific encrypted binary file
    QString userSymptomsFile = getCurrentDataDirectory() + "/symptoms.dat";
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Serialize to binary
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    out << static_cast<qint32>(symptoms.size());

    for (const Symptom &s: symptoms) {
        out << s.getName() << Symptom::typeToString(s.getType()) << s.getUnit();
    }

    DataEncryption::saveEncrypted(userSymptomsFile, data, password);
}

void MainWindow::rebuildSymptomWidgets() {
    // Clear existing widgets
    qDeleteAll(symptomWidgets);
    symptomWidgets.clear();

    // Create new widgets
    for (const Symptom &s: symptoms) {
        SymptomWidget *widget = new SymptomWidget(s);
        symptomWidgets.append(widget);
        symptomsLayout->addWidget(widget);
    }

    symptomsLayout->addStretch();
}

void MainWindow::showAddSymptomDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Symptom");
    dialog.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Name
    layout->addWidget(new QLabel("Symptom Name:"));
    QLineEdit *nameEdit = new QLineEdit();
    layout->addWidget(nameEdit);

    // Type
    layout->addWidget(new QLabel("Type:"));
    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItem("Yes/No (Binary)", QVariant::fromValue(SymptomType::Binary));
    typeCombo->addItem("Count (e.g., times woke up)", QVariant::fromValue(SymptomType::Count));
    typeCombo->addItem("Quantity (e.g., drinks, mg)", QVariant::fromValue(SymptomType::Quantity));
    layout->addWidget(typeCombo);

    // Unit
    layout->addWidget(new QLabel("Unit (optional, for Count/Quantity):"));
    QLineEdit *unitEdit = new QLineEdit();
    unitEdit->setPlaceholderText("e.g., times, drinks, mg, hours");
    layout->addWidget(unitEdit);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Add");
    QPushButton *cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (!name.isEmpty()) {
            SymptomType type = typeCombo->currentData().value<SymptomType>();
            QString unit = unitEdit->text().trimmed();

            // Check if already exists
            bool exists = false;
            for (const Symptom &s: symptoms) {
                if (s.getName() == name) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                symptoms.append(Symptom(name, type, unit));
                saveSymptoms();
                rebuildSymptomWidgets();
            } else {
                QMessageBox::information(this, "Duplicate", "This symptom already exists.");
            }
        }
    }
}

void MainWindow::onAddSymptom() {
    showAddSymptomDialog();
}

void MainWindow::onClearForm() {
    notesEdit->clear();
    dateEdit->setDate(QDate::currentDate());
    bedtimeEdit->setTime(QTime(22, 0));
    wakeupEdit->setTime(QTime(7, 0));

    for (SymptomWidget *widget: symptomWidgets) {
        widget->reset();
    }
}

// QString MainWindow::getCurrentDataDirectory() {
//     QString username = "default";
//
//     if (UserManager::instance().isLoggedIn()) {
//         User *user = UserManager::instance().getCurrentUser();
//         if (user) {
//             username = user->getUsername();
//         }
//     }
//
//     QString dataDir = QString("sleep_data/%1").arg(username);
//     QDir dir;
//     if (!dir.exists(dataDir)) {
//         dir.mkpath(dataDir);
//     }
//     return dataDir;
// }




QString MainWindow::getCurrentDataDirectory() {
    QString username = "default";

    if (UserManager::instance().isLoggedIn()) {
        User *user = UserManager::instance().getCurrentUser();
        if (user) {
            username = user->getUsername();
        }
    }

    return DataPathManager::instance().getUserDataPath(username);
}

QString MainWindow::getSymptomDataFile() {
    QString dataDir = getCurrentDataDirectory();
    return QString("%1/symptom_history.dat").arg(dataDir);
}

bool MainWindow::saveEntry() {
    if (!UserManager::instance().isLoggedIn()) {
        QMessageBox::warning(this, "Not Logged In", "Please login to save entries.");
        return false;
    }

    QString dataDir = getCurrentDataDirectory();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Create filename with date
    QString filename = QString("%1/sleep_%2.dat")
            .arg(dataDir)
            .arg(dateEdit->date().toString("yyyy-MM-dd"));

    // Calculate sleep duration
    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

    // Collect symptom data
    QList<QPair<QString, double> > symptomData;
    for (SymptomWidget *widget: symptomWidgets) {
        Symptom s = widget->getSymptom();
        if (s.isPresent()) {
            symptomData.append(qMakePair(s.getName(), s.getValue()));
        }
    }

    QString notes = notesEdit->toPlainText();

    // Serialize to binary
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    out << QDateTime::currentDateTime()
            << dateEdit->date()
            << bedtimeEdit->time()
            << wakeupEdit->time()
            << hours
            << notes
            << symptomData;

    if (!DataEncryption::saveEncrypted(filename, data, password)) {
        QMessageBox::critical(this, "Error", "Could not save entry!");
        return false;
    }

    return saveSummaryEntry();
}

bool MainWindow::saveSummaryEntry() {
    QString symptomFile = getSymptomDataFile();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Load existing data
    QList<QVariantMap> entries;
    QByteArray existingData = DataEncryption::loadEncrypted(symptomFile, password);

    if (!existingData.isEmpty()) {
        QDataStream in(&existingData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);
        in >> entries;
    }

    // Calculate sleep duration
    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

    // Create new entry
    QVariantMap entry;
    entry["date"] = dateEdit->date();
    entry["sleep_duration"] = hours;

    // Add symptom values
    for (SymptomWidget *widget: symptomWidgets) {
        Symptom s = widget->getSymptom();
        entry[s.getName()] = s.getValue();
    }

    entries.append(entry);

    // Serialize and save
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << entries;

    return DataEncryption::saveEncrypted(symptomFile, data, password);
}

void MainWindow::onSaveEntry() {
    if (saveEntry()) {
        QMessageBox::information(this, "Success", "Sleep entry saved successfully!");
        onClearForm();
        // Refresh history if on that tab
        if (tabWidget->currentIndex() == 1) {
            loadHistoryData();
        }
    }
}
