//created by drmrsthemonarch with ai effort
#include "mainwindow.h"
#include <QToolBar>
#include <QDataStream>
#include "logindialog.h"
#include "histogramwidget.h"
#include "datapathmanager.h"
#include "wordcloudwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    createUserToolbar();

    if (UserManager::instance().isLoggedIn()) {
        loadSymptoms();
        rebuildSymptomWidgets();
    }

    updateWindowTitle();

    // Connect immediately, not with a singleShot
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

MainWindow::~MainWindow() {
    if (UserManager::instance().isLoggedIn()) {
        saveSymptoms();
    }
}

bool MainWindow::createSummaryEntry(const QUuid &entryId, const QDate &date, double duration,
                                    const QList<QPair<QString, double>> &symptomData) {
    QString symptomFile = getSymptomDataFile();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Load existing summary entries
    QList<QVariantMap> entries;
    QByteArray existingData = DataEncryption::loadEncrypted(symptomFile, password);

    if (!existingData.isEmpty()) {
        QDataStream in(&existingData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);
        in >> entries;
    }

    // Create new entry
    QVariantMap entry;
    entry["id"] = entryId.toString(QUuid::WithoutBraces);
    entry["date"] = date;
    entry["sleep_duration"] = duration;

    // Add symptom values
    for (const auto &pair : symptomData) {
        entry[pair.first] = pair.second;
    }

    // Add to entries
    entries.append(entry);

    // Save updated entries back to summary file
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << entries;

    return DataEncryption::saveEncrypted(symptomFile, data, password);
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

void MainWindow::exportHistoryToCSV(const QString &filename, const QList<QVariantMap> &entries) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"),
                             tr("Could not open file for writing: %1").arg(filename));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // Load data from summary file instead of individual entries
    QList<QVariantMap> summaryData = loadSummaryData();

    if (summaryData.isEmpty()) {
        QMessageBox::warning(this, tr("Export Warning"), tr("No summary data found to export"));
        file.close();
        return;
    }

    // Get all unique field names from summary data
    QSet<QString> allFieldNames;
    for (const auto &entry: summaryData) {
        for (auto it = entry.begin(); it != entry.end(); ++it) {
            allFieldNames.insert(it.key());
        }
    }

    QStringList fieldNames = allFieldNames.toList();
    fieldNames.sort();

    // Ensure date comes first if present
    if (fieldNames.contains("date")) {
        fieldNames.removeAll("date");
        fieldNames.prepend("date");
    }

    // Write CSV header
    QStringList headers;
    for (const QString &field: fieldNames) {
        QString header = field;
        header = header.replace("_", " ");
        header[0] = header[0].toUpper();
        headers << header;
    }

    out << headers.join(",") << "\n";

    // Write data rows
    for (const auto &entry: summaryData) {
        QStringList row;

        for (const QString &field: fieldNames) {
            QString value;

            if (entry.contains(field)) {
                QVariant fieldValue = entry.value(field);
                if (fieldValue.type() == QVariant::Date) {
                    value = fieldValue.toDate().toString("yyyy-MM-dd");
                } else if (fieldValue.type() == QVariant::DateTime) {
                    value = fieldValue.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
                } else if (fieldValue.type() == QVariant::Time) {
                    value = fieldValue.toTime().toString("hh:mm:ss");
                } else {
                    value = fieldValue.toString();
                }
            }

            // Escape commas and quotes in CSV values
            if (value.contains(",") || value.contains("\"")) {
                value = "\"" + value.replace("\"", "\"\"") + "\"";
            }

            row << (value.isEmpty() ? "0" : value);
        }

        out << row.join(",") << "\n";
    }

    file.close();

    QMessageBox::information(this, tr("Export Complete"),
                             tr("History exported successfully to:\n%1\n\n%2 entries exported with %3 fields")
                             .arg(filename)
                             .arg(summaryData.size())
                             .arg(fieldNames.size()));
}

void MainWindow::initializeHistogram() const {
    // Enable mouse tracking
    histogramCustomPlot->setMouseTracking(true);

    // Configure zooming behavior
    histogramCustomPlot->axisRect()->setRangeZoomFactor(0.85); // Slower zoom
    histogramCustomPlot->axisRect()->setRangeDrag(nullptr); // Start with no drag

    // Add zoom instructions to tooltip
    histogramCustomPlot->setToolTip("Scroll: Vertical zoom\nDrag: Horizontal scroll\nRight-click: Context menu");
}

QString MainWindow::getCurrentDataDirectory() {
    QString username = "default";

    if (UserManager::instance().isLoggedIn()) {
        if (User *user = UserManager::instance().getCurrentUser()) {
            username = user->getUsername();
        }
    }

    return DataPathManager::instance().getUserDataPath(username);
}

QString MainWindow::getSymptomDataFile() {
    QString dataDir = getCurrentDataDirectory();
    return QString("%1/symptom_history.dat").arg(dataDir);
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

void MainWindow::loadHistogramData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    if (histogramSymptomListWidget->count() == 0) {
        auto sleepItem = new QListWidgetItem("Sleep Duration");
        sleepItem->setFlags(sleepItem->flags() | Qt::ItemIsUserCheckable);
        sleepItem->setCheckState(Qt::Checked);
        histogramSymptomListWidget->addItem(sleepItem);

        for (const Symptom &s: symptoms) {
            auto item = new QListWidgetItem(s.getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            histogramSymptomListWidget->addItem(item);
        }
    }

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

    // Check if entries have IDs, if not, add them (migration on-the-fly)
    bool needsMigration = false;
    for (auto &entry : allEntries) {
        if (!entry.contains("id")) {
            // This is an old entry without ID, generate one
            entry["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
            needsMigration = true;
        }
    }

    // Save back if migration was needed
    if (needsMigration) {
        QByteArray updatedData;
        QDataStream out(&updatedData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_15);
        out << allEntries;
        DataEncryption::saveEncrypted(symptomFile, updatedData, password);
    }

    return allEntries;
}

QList<QVariantMap> MainWindow::loadSummaryData() {
    return loadAllEntries();
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
        QString entryId = entry["id"].toString();
        QDate entryDate = entry["date"].toDate();

        // Date
        auto *dateItem = new QTableWidgetItem(entryDate.toString("yyyy-MM-dd"));
        dateItem->setData(Qt::UserRole, entryId); // Store ID in item data
        historyTable->setItem(i, 0, dateItem);

        // Sleep duration
        double duration = entry["sleep_duration"].toDouble();
        auto *durationItem = new QTableWidgetItem(QString::number(duration, 'f', 1) + " hrs");
        historyTable->setItem(i, 1, durationItem);

        // Try to load detailed data using ID first, then fall back to date
        QString dataDir = getCurrentDataDirectory();
        QString idBasedFilename = QString("%1/sleep_%2.dat").arg(dataDir).arg(entryId);
        QString dateBasedFilename = QString("%1/sleep_%2.dat").arg(dataDir).arg(entryDate.toString("yyyy-MM-dd"));

        QString bedtimeStr = "-";
        QString waketimeStr = "-";

        QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

        // Try ID-based filename first (new system)
        QByteArray dailyData = DataEncryption::loadEncrypted(idBasedFilename, password);

        // If not found, try date-based filename (old system)
        if (dailyData.isEmpty()) {
            dailyData = DataEncryption::loadEncrypted(dateBasedFilename, password);
        }

        if (!dailyData.isEmpty()) {
            QDataStream in(&dailyData, QIODevice::ReadOnly);
            in.setVersion(QDataStream::Qt_5_15);

            QUuid id;
            QDateTime timestamp;
            QDate date;
            QTime bedtime, waketime;

            in >> id >> timestamp >> date >> bedtime >> waketime;

            bedtimeStr = bedtime.toString("HH:mm");
            waketimeStr = waketime.toString("HH:mm");

            // If this was an old file without ID, migrate it now
            if (id.isNull()) {
                id = QUuid::createUuid();
                // ... save with new ID format (similar to saveEntry logic)
            }
        }

        historyTable->setItem(i, 2, new QTableWidgetItem(bedtimeStr));
        historyTable->setItem(i, 3, new QTableWidgetItem(waketimeStr));

        // Symptoms (your existing code)
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

        auto *symptomsItem = new QTableWidgetItem(symptomList.join(", "));
        historyTable->setItem(i, 4, symptomsItem);
    }

    historyTable->resizeColumnsToContents();
}

void MainWindow::loadStatisticsData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    if (symptomListWidget->count() == 0) {
        auto sleepItem = new QListWidgetItem("Sleep Duration");
        sleepItem->setFlags(sleepItem->flags() | Qt::ItemIsUserCheckable);
        sleepItem->setCheckState(Qt::Checked);
        symptomListWidget->addItem(sleepItem);

        for (const Symptom &s: symptoms) {
            auto item = new QListWidgetItem(s.getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            symptomListWidget->addItem(item);
        }
    }

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

    switch (int plotType = plotTypeSelector->currentIndex()) {
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

void MainWindow::loadWordCloudData() {
    if (!UserManager::instance().isLoggedIn()) {
        return;
    }

    // Load all entries
    QList<QVariantMap> entries = loadAllEntries();

    if (entries.isEmpty()) {
        wordCloudWidget->setWordFrequencies(QMap<QString, int>());
        wordCountLabel->setText("Total words: 0");
        QMessageBox::information(this, "No Data", "No sleep data available for word cloud analysis.");
        return;
    }

    // Filter by date range if needed
    if (!wordCloudAllDateRangeCheckbox->isChecked()) {
        entries = filterEntriesByDateRange(entries, wordCloudStartDateEdit->date(), wordCloudEndDateEdit->date());
        if (entries.isEmpty()) {
            wordCloudWidget->setWordFrequencies(QMap<QString, int>());
            wordCountLabel->setText("Total words: 0");
            QMessageBox::information(this, "No Data", "No data in selected date range.");
            return;
        }
    }

    // Extract text from all notes
    QString allNotesText;
    int entriesWithNotes = 0;

    QString dataDir = getCurrentDataDirectory();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    for (const QVariantMap &entry: entries) {
        QDate entryDate = entry["date"].toDate();
        QString filename = QString("%1/sleep_%2.dat")
                .arg(dataDir)
                .arg(entryDate.toString("yyyy-MM-dd"));

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

            if (!notes.trimmed().isEmpty()) {
                allNotesText += notes + " ";
                entriesWithNotes++;
            }
        }
    }

    if (allNotesText.trimmed().isEmpty()) {
        wordCloudWidget->setWordFrequencies(QMap<QString, int>());
        wordCountLabel->setText("Total words: 0");
        return;
    }

    // Process text and count word frequencies
    QMap<QString, int> wordFrequencies;

    // Simple word extraction and cleaning
    QStringList words = allNotesText.toLower().split(QRegExp("\\W+"), QString::SkipEmptyParts);

    QStringList stopWords = WordCloudWidget::getStopWords();

    for (const QString &word: words) {
        // Filter out very short words and stop words
        if (word.length() >= 3 && !stopWords.contains(word)) {
            wordFrequencies[word]++;
        }
    }

    // Update word cloud
    wordCloudWidget->setMinimumFrequency(minWordFrequencySpinBox->value());
    wordCloudWidget->setMaxWords(maxWordsSpinBox->value());
    wordCloudWidget->setWordFrequencies(wordFrequencies);

    // Update statistics
    int totalUniqueWords = wordFrequencies.size();
    int totalWords = words.size();
    wordCountLabel->setText(QString("Total unique words: %1 (from %2 total words in %3 entries)")
        .arg(totalUniqueWords)
        .arg(totalWords)
        .arg(entriesWithNotes));
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

void MainWindow::onUserChanged() {
    if (bool loggedIn = UserManager::instance().isLoggedIn()) {
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
            // User canceled login, close the application
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
    } else if (index == 4) {
        // Word Cloud tab
        loadWordCloudData();
    }
}

void MainWindow::onHistoryDateSelected(int row, int column) {
    Q_UNUSED(column);

    QTableWidgetItem *dateItem = historyTable->item(row, 0);
    if (!dateItem) return;

    // Try to get the entry ID from UserRole (new system)
    QString entryId = dateItem->data(Qt::UserRole).toString();
    QDate entryDate = QDate::fromString(dateItem->text(), "yyyy-MM-dd");

    QString dataDir = getCurrentDataDirectory();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    QByteArray data;
    QUuid id;
    QDateTime timestamp;
    QDate date;
    QTime bedtime, waketime;
    double hours;
    QString notes;
    QList<QPair<QString, double>> symptomData;

    if (!entryId.isEmpty()) {
        // New system: Load by ID
        QString filename = QString("%1/sleep_%2.dat").arg(dataDir).arg(entryId);
        data = DataEncryption::loadEncrypted(filename, password);

        if (!data.isEmpty()) {
            QDataStream in(&data, QIODevice::ReadOnly);
            in.setVersion(QDataStream::Qt_5_15);
            in >> id >> timestamp >> date >> bedtime >> waketime >> hours >> notes >> symptomData;
        }
    }

    // If not found with new system, try old system (date-based)
    if (data.isEmpty()) {
        QString oldFilename = QString("%1/sleep_%2.dat").arg(dataDir).arg(entryDate.toString("yyyy-MM-dd"));
        data = DataEncryption::loadEncrypted(oldFilename, password);
        QByteArray oldData = DataEncryption::loadEncrypted(oldFilename, password);
        // If we found an old entry, migrate it to the new system
if (!oldData.isEmpty()) {
            // Read old data
            QDataStream oldIn(&oldData, QIODevice::ReadOnly);
            oldIn.setVersion(QDataStream::Qt_5_15);
            oldIn >> timestamp >> date >> bedtime >> waketime >> hours >> notes >> symptomData;

            // Generate new ID for migration
            id = QUuid::createUuid();
            QString newFilename = QString("%1/sleep_%2.dat").arg(dataDir).arg(id.toString(QUuid::WithoutBraces));

            // Write with ID to new file
            QByteArray newData;
            QDataStream newOut(&newData, QIODevice::WriteOnly);
            newOut.setVersion(QDataStream::Qt_5_15);
            newOut << id << timestamp << date << bedtime << waketime << hours << notes << symptomData;

            // Save new file
            if (DataEncryption::saveEncrypted(newFilename, newData, password)) {
                // Update the table to store the new ID
                dateItem->setData(Qt::UserRole, id.toString(QUuid::WithoutBraces));

                // Also update the summary file - CREATE the entry if it doesn't exist
                if (!updateSummaryEntry(id, date, hours, symptomData)) {
                    // If update failed, create new summary entry
                    createSummaryEntry(id, date, hours, symptomData);
                }

                // Optionally remove the old file after successful migration
                QFile::remove(oldFilename);

                // Use the new data for the dialog
                data = newData;
                entryId = id.toString(QUuid::WithoutBraces);
            }
        }
    }

    if (!data.isEmpty()) {
        QDataStream in(&data, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);

        QUuid id;
        QDateTime timestamp;
        QDate date;
        QTime bedtime, waketime;
        double hours;
        QString notes;
        QList<QPair<QString, double>> symptomData;

        in >> id >> timestamp >> date >> bedtime >> waketime >> hours >> notes >> symptomData;

        // Show editable details dialog (the rest of your existing code remains the same)
        QDialog dialog(this);
        dialog.setWindowTitle(QString("Edit Sleep Entry - %1").arg(date.toString("yyyy-MM-dd")));
        dialog.resize(600, 700);
        auto layout = new QVBoxLayout(&dialog);

        // Date (EDITABLE now)
        layout->addWidget(new QLabel("<b>Date:</b>"));
        auto dateEdit = new QDateEdit(date);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat("yyyy-MM-dd");
        layout->addWidget(dateEdit);

        // Bedtime (editable)
        layout->addWidget(new QLabel("<b>Bedtime:</b>"));
        auto bedtimeEdit = new QTimeEdit(bedtime);
        bedtimeEdit->setDisplayFormat("HH:mm");
        layout->addWidget(bedtimeEdit);

        // Wake time (editable)
        layout->addWidget(new QLabel("<b>Wake time:</b>"));
        auto waketimeEdit = new QTimeEdit(waketime);
        waketimeEdit->setDisplayFormat("HH:mm");
        layout->addWidget(waketimeEdit);

        // Sleep duration (auto-calculated)
        auto durationLabel = new QLabel(QString("<b>Sleep duration:</b> %1 hours").arg(hours, 0, 'f', 1));
        layout->addWidget(durationLabel);

        // Auto-update duration when times OR DATE changes
        auto updateDuration = [=]() {
            QDate newDate = dateEdit->date();
            QTime bed = bedtimeEdit->time();
            QTime wake = waketimeEdit->time();

            // Calculate duration considering overnight sleep
            QDateTime bedDateTime(newDate, bed);
            QDateTime wakeDateTime(newDate.addDays(1), wake);
            if (wake > bed) {
                wakeDateTime = QDateTime(newDate, wake);
            }

            double newHours = bedDateTime.secsTo(wakeDateTime) / 3600.0;
            durationLabel->setText(QString("<b>Sleep duration:</b> %1 hours").arg(newHours, 0, 'f', 1));
        };

        connect(dateEdit, &QDateEdit::dateChanged, updateDuration);
        connect(bedtimeEdit, QOverload<const QTime &>::of(&QTimeEdit::timeChanged), updateDuration);
        connect(waketimeEdit, QOverload<const QTime &>::of(&QTimeEdit::timeChanged), updateDuration);

        // Notes (editable)
        layout->addWidget(new QLabel("<b>Notes:</b>"));
        auto notesEdit = new QTextEdit();
        notesEdit->setPlainText(notes);
        notesEdit->setMaximumHeight(120);
        layout->addWidget(notesEdit);

        // Symptoms (editable with add/remove capabilities)
        layout->addWidget(new QLabel("<b>Symptoms:</b>"));

        // Create a map from current symptom data for quick lookup
        QMap<QString, double> currentSymptomValues;
        for (const auto &pair: symptomData) {
            currentSymptomValues[pair.first] = pair.second;
        }

        // Symptoms scroll area
        auto symptomsScrollArea = new QScrollArea();
        auto symptomsWidget = new QWidget();
        auto symptomsLayout = new QVBoxLayout(symptomsWidget);

        // Create symptom widgets for ALL available symptoms
        QList<QWidget *> symptomEditWidgets;
        QList<QCheckBox *> symptomCheckboxes;

        for (const Symptom &symptom: symptoms) {
            auto symptomLayout = new QHBoxLayout();
            // Checkbox to enable/disable the symptom
            auto enabledCheckBox = new QCheckBox();
            enabledCheckBox->setChecked(currentSymptomValues.contains(symptom.getName()));
            symptomLayout->addWidget(enabledCheckBox);
            symptomCheckboxes.append(enabledCheckBox);

            // Symptom name
            auto nameLabel = new QLabel(symptom.getName() + ":");
            nameLabel->setMinimumWidth(150);
            symptomLayout->addWidget(nameLabel);

            // Value input based on symptom type
            QWidget *valueWidget = nullptr;
            if (symptom.getType() == SymptomType::Binary) {
                auto comboBox = new QComboBox();
                comboBox->addItems({"No", "Yes"});
                if (currentSymptomValues.contains(symptom.getName())) {
                    comboBox->setCurrentIndex(currentSymptomValues[symptom.getName()] > 0 ? 1 : 0);
                }
                comboBox->setEnabled(enabledCheckBox->isChecked());
                valueWidget = comboBox;

                // Connect checkbox to enable/disable the combo
                connect(enabledCheckBox, &QCheckBox::toggled, comboBox, &QComboBox::setEnabled);
            } else {
                auto spinBox = new QDoubleSpinBox();
                spinBox->setRange(0.0, 9999.0);
                spinBox->setSingleStep(0.1);
                spinBox->setDecimals(1);
                if (currentSymptomValues.contains(symptom.getName())) {
                    spinBox->setValue(currentSymptomValues[symptom.getName()]);
                }
                spinBox->setEnabled(enabledCheckBox->isChecked());

                if (!symptom.getUnit().isEmpty()) {
                    spinBox->setSuffix(" " + symptom.getUnit());
                }
                valueWidget = spinBox;

                // Connect checkbox to enable/disable the spinbox
                connect(enabledCheckBox, &QCheckBox::toggled, spinBox, &QDoubleSpinBox::setEnabled);
            }

            symptomLayout->addWidget(valueWidget);
            symptomLayout->addStretch();

            auto symptomWidget = new QWidget();
            symptomWidget->setLayout(symptomLayout);
            symptomEditWidgets.append(symptomWidget);
            symptomsLayout->addWidget(symptomWidget);
        }

        symptomsScrollArea->setWidget(symptomsWidget);
        symptomsScrollArea->setWidgetResizable(true);
        symptomsScrollArea->setMaximumHeight(250);
        layout->addWidget(symptomsScrollArea);

        // Buttons
        auto buttonLayout = new QHBoxLayout();
        auto saveButton = new QPushButton("Save Changes");
        saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px 16px;");
        auto cancelButton = new QPushButton("Cancel");
        cancelButton->setStyleSheet("background-color: #f44336; color: white; padding: 8px 16px;");

        buttonLayout->addStretch();
        buttonLayout->addWidget(saveButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);

        // Connect buttons
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        connect(saveButton, &QPushButton::clicked, [&, id, originalDate = date]() {
            // Collect updated data
            QDate newDate = dateEdit->date();
            QTime newBedtime = bedtimeEdit->time();
            QTime newWaketime = waketimeEdit->time();
            QString newNotes = notesEdit->toPlainText();

            // Calculate new duration
            QDateTime bedDateTime(newDate, newBedtime);
            QDateTime wakeDateTime(newDate.addDays(1), newWaketime);
            if (newWaketime > newBedtime) {
                wakeDateTime = QDateTime(newDate, newWaketime);
            }
            double newHours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

            // Collect symptom data from ALL symptoms (only enabled ones)
            QList<QPair<QString, double>> newSymptomData;
            for (int i = 0; i < symptoms.size() && i < symptomEditWidgets.size(); ++i) {
                QCheckBox *checkBox = symptomCheckboxes[i];
                if (!checkBox->isChecked()) {
                    continue; // Skip disabled symptoms
                }

                const Symptom &symptom = symptoms[i];
                QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(symptomEditWidgets[i]->layout());
                if (layout && layout->count() >= 3) {
                    QWidget *valueWidget = layout->itemAt(2)->widget();
                    double value = 0.0;

                    if (symptom.getType() == SymptomType::Binary) {
                        if (QComboBox *comboBox = qobject_cast<QComboBox *>(valueWidget)) {
                            value = comboBox->currentIndex(); // 0 for "No", 1 for "Yes"
                        }
                    } else {
                        if (QDoubleSpinBox *spinBox = qobject_cast<QDoubleSpinBox *>(valueWidget)) {
                            value = spinBox->value();
                        }
                    }

                    newSymptomData.append(qMakePair(symptom.getName(), value));
                }
            }

            // Save updated data to daily file (same filename since we use ID)
            QString updatedFilename = QString("%1/sleep_%2.dat").arg(dataDir).arg(id.toString(QUuid::WithoutBraces));
            QByteArray newData;
            QDataStream out(&newData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_15);
            out << id << timestamp << newDate << newBedtime << newWaketime << newHours << newNotes << newSymptomData;

            bool dailyFileSaved = DataEncryption::saveEncrypted(updatedFilename, newData, password);

            // IMPORTANT: Also update the summary file using the ID
            bool summaryFileSaved = updateSummaryEntry(id, newDate, newHours, newSymptomData);

            if (dailyFileSaved && summaryFileSaved) {
                QMessageBox::information(&dialog, "Success", "Entry updated successfully!");
                dialog.accept();
                // Refresh the history table and any open plots
                loadHistoryData();
                // If statistics tab is open, refresh it too
                if (tabWidget->currentIndex() == 2) {
                    // Statistics tab index
                    loadStatisticsData();
                }
                // If histogram tab is open, refresh it too
                if (tabWidget->currentIndex() == 3) {
                    // Histogram tab index
                    loadHistogramData();
                }
            } else {
                QMessageBox::critical(&dialog, "Error", "Failed to save changes!");
            }
        });

        dialog.exec();
    } else {
        QMessageBox::warning(this, "Error", "Could not load the selected entry.");
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

    QString entryId = dateItem->data(Qt::UserRole).toString();
    QDate selectedDate = QDate::fromString(dateItem->text(), "yyyy-MM-dd");

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Entry",
        QString("Are you sure you want to delete the entry for %1?").arg(selectedDate.toString("yyyy-MM-dd")),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Delete the daily file using ID
        QString dataDir = getCurrentDataDirectory();
        QString filename = QString("%1/sleep_%2.dat").arg(dataDir).arg(entryId);
        QFile::remove(filename);

        // Remove from summary file using ID
        QList<QVariantMap> entries = loadAllEntries();
        QList<QVariantMap> filteredEntries;
        for (const auto &entry : entries) {
            if (entry["id"].toString() != entryId) {
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

void MainWindow::onPlotTypeChanged(int index) {
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

void MainWindow::onAddSymptom() {
    showAddSymptomDialog();
}

void MainWindow::onClearForm() {
    notesEdit->clear();
    dateEdit->setDate(QDate::currentDate());
    bedtimeEdit->setTime(QTime(01, 0));
    wakeupEdit->setTime(QTime(8, 30));

    for (SymptomWidget *widget: symptomWidgets) {
        widget->reset();
    }
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

QStringList MainWindow::parseCSVLine(const QString &line) {
    QStringList values;
    QString currentValue;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line[i];

        if (ch == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // Escaped quote
                currentValue += '"';
                ++i; // Skip next quote
            } else {
                // Toggle quote mode
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            // End of field
            values.append(currentValue.trimmed());
            currentValue.clear();
        } else {
            currentValue += ch;
        }
    }

    // Add the last value
    values.append(currentValue.trimmed());

    return values;
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

    double xBuffer = entries.size() * 0.1;
    customPlot->xAxis->setRange(-xBuffer, entries.size() - 1 + xBuffer);
    customPlot->rescaleAxes();

    // Add some padding to y-axis as well (10% on top)
    QCPRange yRange = customPlot->yAxis->range();
    double yPadding = yRange.size() * 0.5;
    customPlot->yAxis->setRange(yRange.lower - yPadding * 0.1, yRange.upper + yPadding);

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

    double xBuffer = (dateNumbers.last() - dateNumbers.first()) * 0.1;
    customPlot->xAxis->setRange(dateNumbers.first() - xBuffer, dateNumbers.last() + xBuffer);
    customPlot->yAxis->setRange(0, maxValue * 1.5);

    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("MMM d\nyyyy");
    customPlot->xAxis->setTicker(dateTicker);

    customPlot->legend->setVisible(true);
    customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    customPlot->replot();
}

void MainWindow::plotCorrelationData(const QList<QVariantMap> &entries, const QStringList &selectedSymptoms) {
    customPlot->clearGraphs();
    customPlot->clearPlottables();

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
        dateNumbers.append(QDateTime(date).toMSecsSinceEpoch() / 1000.0);

    double totalSeconds = dateNumbers.last() - dateNumbers.first();
    double minDate = dateNumbers.first();
    double maxDate = dateNumbers.last();
    double xBuffer = (maxDate - minDate) * 0.1;
    double barWidth = totalSeconds / (dates.size() * selectedSymptoms.size() * 1.5);

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
        auto axisRect = new QCPAxisRect(histogramCustomPlot);
        histogramCustomPlot->plotLayout()->addElement(i, 0, axisRect);

        // Configure axis rect for zooming and scrolling
        axisRect->setRangeDrag(Qt::Horizontal); // Allow horizontal dragging (scrolling)
        axisRect->setRangeZoom(Qt::Vertical); // Allow vertical zooming
        axisRect->setRangeZoomAxes(axisRect->axis(QCPAxis::atBottom), axisRect->axis(QCPAxis::atLeft));

        QCPAxis *xAxis = axisRect->axis(QCPAxis::atBottom);
        QCPAxis *yAxis = axisRect->axis(QCPAxis::atLeft);
        synchronizedXAxes.append(xAxis);

        auto bars = new QCPBars(xAxis, yAxis);
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
        yAxis->setRange(0, maxY[i] * 1.25);

        // Set initial X range with some padding
        xAxis->setRange(minDate - xBuffer, maxDate + xBuffer);

        auto title = new QCPTextElement(histogramCustomPlot, selectedSymptoms[i]);
        axisRect->insetLayout()->addElement(title, Qt::AlignTop | Qt::AlignHCenter);

        // Add margin to prevent title overlap
        axisRect->setAutoMargins(QCP::msLeft | QCP::msRight | QCP::msBottom);
        axisRect->setMargins(QMargins(50, 40, 50, 30));
    }

    // Connect synchronization for all x-axes
    for (int i = 0; i < synchronizedXAxes.size(); ++i) {
        if (QPointer<QCPAxis> xAxis = synchronizedXAxes[i]) {
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

void MainWindow::resetHistogramZoom() {
    // Replot with original ranges
    // You might want to store the original date ranges and replot
    histogramCustomPlot->rescaleAxes();
    histogramCustomPlot->replot();
}

void MainWindow::rebuildSymptomWidgets() {
    // Clear existing widgets
    qDeleteAll(symptomWidgets);
    symptomWidgets.clear();

    // Create new widgets
    for (const Symptom &s: symptoms) {
        auto widget = new SymptomWidget(s);
        symptomWidgets.append(widget);
        symptomsLayout->addWidget(widget);
    }

    symptomsLayout->addStretch();
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

bool MainWindow::saveEntry() {
    if (!UserManager::instance().isLoggedIn()) {
        QMessageBox::warning(this, "Not Logged In", "Please login to save entries.");
        return false;
    }

    QString dataDir = getCurrentDataDirectory();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Generate unique filename with ID instead of date
    QUuid entryId = QUuid::createUuid();
    QString filename = QString("%1/sleep_%2.dat")
            .arg(dataDir)
            .arg(entryId.toString(QUuid::WithoutBraces));

    // Calculate sleep duration
    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

    // Collect symptom data
    QList<QPair<QString, double>> symptomData;
    for (SymptomWidget *widget : symptomWidgets) {
        Symptom s = widget->getSymptom();
        if (s.isPresent()) {
            symptomData.append(qMakePair(s.getName(), s.getValue()));
        }
    }

    QString notes = notesEdit->toPlainText();

    // Serialize to binary - include ID
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);

    out << entryId
        << QDateTime::currentDateTime()
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

    return saveSummaryEntry(entryId);
}

bool MainWindow::saveSummaryEntry(const QUuid &entryId) {
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

    // Create new entry with ID
    QVariantMap entry;
    QDate currentDate = dateEdit->date();
    entry["id"] = entryId.toString(QUuid::WithoutBraces); // Store ID as string
    entry["date"] = currentDate;
    entry["sleep_duration"] = hours;

    // Add symptom values
    for (SymptomWidget *widget : symptomWidgets) {
        Symptom s = widget->getSymptom();
        entry[s.getName()] = s.getValue();
    }

    // Check if entry with this ID already exists
    bool entryExists = false;
    int existingIndex = -1;
    QString entryIdString = entryId.toString(QUuid::WithoutBraces);

    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i]["id"].toString() == entryIdString) {
            entryExists = true;
            existingIndex = i;
            break;
        }
    }

    if (entryExists) {
        // Update existing entry
        entries[existingIndex] = entry;
    } else {
        // Add new entry
        entries.append(entry);
    }

    // Serialize and save
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << entries;

    return DataEncryption::saveEncrypted(symptomFile, data, password);
}

void MainWindow::setupUI() {
    setWindowTitle("Sleepbook");
    resize(1200, 800);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);

    // Create tab widget
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);

    // Create tabs
    setupEntryTab();
    setupHistoryTab();
    setupStatisticsTab();
    setupHistogramTab();
    setupWordCloudTab();

    tabWidget->addTab(entryTab, "New Entry");
    tabWidget->addTab(historyTab, "History");
    tabWidget->addTab(statisticsTab, "Plots");
    tabWidget->addTab(histogramTab, "Histograms");
    tabWidget->addTab(wordCloudTab, "Word Cloud");

    // Connect signal
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Pre-load data for the initially visible tab
    QTimer::singleShot(0, this, [this]() {
        onTabChanged(tabWidget->currentIndex());
    });
}

void MainWindow::setupHistogramTab() {
    histogramTab = new QWidget();
    auto *mainLayout = new QHBoxLayout(histogramTab);

    // Left panel - controls
    auto *controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(10);

    auto *titleLabel = new QLabel("Stacked Histogram Configuration");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(titleLabel);

    // Date range
    controlLayout->addWidget(new QLabel("Date Range:"));
    histogramAllDateRangeCheckbox = new QCheckBox("Use all data");
    histogramAllDateRangeCheckbox->setChecked(true);
    controlLayout->addWidget(histogramAllDateRangeCheckbox);

    auto *startDateLayout = new QHBoxLayout();
    startDateLayout->addWidget(new QLabel("From:"));
    histogramStartDateEdit = new QDateEdit();
    histogramStartDateEdit->setDate(QDate::currentDate().addMonths(-1));
    histogramStartDateEdit->setCalendarPopup(true);
    histogramStartDateEdit->setEnabled(false);
    startDateLayout->addWidget(histogramStartDateEdit);
    controlLayout->addLayout(startDateLayout);

    auto *endDateLayout = new QHBoxLayout();
    endDateLayout->addWidget(new QLabel("To:"));
    histogramEndDateEdit = new QDateEdit();
    histogramEndDateEdit->setDate(QDate::currentDate());
    histogramEndDateEdit->setCalendarPopup(true);
    histogramEndDateEdit->setEnabled(false);
    endDateLayout->addWidget(histogramEndDateEdit);
    controlLayout->addLayout(endDateLayout);

    // Symptom selection
    controlLayout->addWidget(new QLabel("Select Symptoms/Metrics:"));

    auto selectButtonLayout = new QHBoxLayout();
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
    auto plotLayout = new QVBoxLayout();

    auto plotTitle = new QLabel("Stacked Histogram Visualization");
    plotTitle->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    plotLayout->addWidget(plotTitle);

    histogramCustomPlot = new QCustomPlot();
    histogramCustomPlot->setOpenGl(true, 4);
    histogramCustomPlot->setMinimumHeight(600);
    plotLayout->addWidget(histogramCustomPlot);

    auto infoLabel = new QLabel(
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
    auto entryLayout = new QHBoxLayout(entryTab);

    // Left Panel - Notes
    auto leftLayout = new QVBoxLayout();
    auto notesLabel = new QLabel("Sleep Notes:");
    notesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(notesLabel);

    // Date and time inputs
    auto dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Date:"));
    dateEdit = new QDateEdit();
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateLayout->addWidget(dateEdit);
    dateLayout->addStretch();
    leftLayout->addLayout(dateLayout);

    auto timeLayout = new QHBoxLayout();
    timeLayout->addWidget(new QLabel("Bedtime:"));
    bedtimeEdit = new QTimeEdit();
    bedtimeEdit->setTime(QTime(01, 0));
    timeLayout->addWidget(bedtimeEdit);

    timeLayout->addWidget(new QLabel("Wake time:"));
    wakeupEdit = new QTimeEdit();
    wakeupEdit->setTime(QTime(8, 30));
    timeLayout->addWidget(wakeupEdit);
    timeLayout->addStretch();
    leftLayout->addLayout(timeLayout);

    notesEdit = new QTextEdit();
    notesEdit->setPlaceholderText("Describe your sleep quality, how you felt, any disturbances, dreams, etc...");
    leftLayout->addWidget(notesEdit);

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save Entry");
    saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px; font-weight: bold;");
    clearButton = new QPushButton("Clear");
    clearButton->setStyleSheet("padding: 8px;");

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(clearButton);
    leftLayout->addLayout(buttonLayout);

    // Right Panel - Symptoms
    auto rightLayout = new QVBoxLayout();
    auto symptomsLabel = new QLabel("Symptoms/Factors:");
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
    auto layout = new QVBoxLayout(historyTab);

    auto titleLabel = new QLabel("Sleep History");
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
    auto buttonLayout = new QHBoxLayout();
    refreshHistoryButton = new QPushButton("Refresh");
    deleteEntryButton = new QPushButton("Delete Selected");
    deleteEntryButton->setStyleSheet("background-color: #f44336; color: white; padding: 6px;");
    auto exportButton = new QPushButton(tr("Export to CSV"), this);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportHistoryToCSV);

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
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
    auto mainLayout = new QHBoxLayout(statisticsTab);

    // Left panel - controls
    auto controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(10);

    auto titleLabel = new QLabel("Plot Configuration");
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

    auto startDateLayout = new QHBoxLayout();
    startDateLayout->addWidget(new QLabel("From:"));
    startDateEdit = new QDateEdit();
    startDateEdit->setDate(QDate::currentDate().addMonths(-1));
    startDateEdit->setCalendarPopup(true);
    startDateEdit->setEnabled(false);
    startDateLayout->addWidget(startDateEdit);
    controlLayout->addLayout(startDateLayout);

    auto endDateLayout = new QHBoxLayout();
    endDateLayout->addWidget(new QLabel("To:"));
    endDateEdit = new QDateEdit();
    endDateEdit->setDate(QDate::currentDate());
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setEnabled(false);
    endDateLayout->addWidget(endDateEdit);
    controlLayout->addLayout(endDateLayout);

    // Symptom selection
    controlLayout->addWidget(new QLabel("Select Symptoms/Metrics:"));

    auto selectButtonLayout = new QHBoxLayout();
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
    auto plotLayout = new QVBoxLayout();

    auto plotTitle = new QLabel("Sleep Statistics Visualization");
    plotTitle->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    plotLayout->addWidget(plotTitle);

    customPlot = new QCustomPlot();
    customPlot->setOpenGl(true, 4);
    customPlot->setMinimumHeight(500);
    plotLayout->addWidget(customPlot);

    auto infoLabel = new QLabel("Tip: Select multiple symptoms to compare. Use mouse wheel to zoom, drag to pan.");
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


void MainWindow::setupWordCloudTab() {
    wordCloudTab = new QWidget();
    auto mainLayout = new QHBoxLayout(wordCloudTab);

    // Left panel - controls
    auto *controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(10);

    auto *titleLabel = new QLabel("Word Cloud Configuration");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    controlLayout->addWidget(titleLabel);

    // Date range
    controlLayout->addWidget(new QLabel("Date Range:"));
    wordCloudAllDateRangeCheckbox = new QCheckBox("Use all data");
    wordCloudAllDateRangeCheckbox->setChecked(true);
    controlLayout->addWidget(wordCloudAllDateRangeCheckbox);

    auto startDateLayout = new QHBoxLayout();
    startDateLayout->addWidget(new QLabel("From:"));
    wordCloudStartDateEdit = new QDateEdit();
    wordCloudStartDateEdit->setDate(QDate::currentDate().addMonths(-1));
    wordCloudStartDateEdit->setCalendarPopup(true);
    wordCloudStartDateEdit->setEnabled(false);
    startDateLayout->addWidget(wordCloudStartDateEdit);
    controlLayout->addLayout(startDateLayout);

    auto endDateLayout = new QHBoxLayout();
    endDateLayout->addWidget(new QLabel("To:"));
    wordCloudEndDateEdit = new QDateEdit();
    wordCloudEndDateEdit->setDate(QDate::currentDate());
    wordCloudEndDateEdit->setCalendarPopup(true);
    wordCloudEndDateEdit->setEnabled(false);
    endDateLayout->addWidget(wordCloudEndDateEdit);
    controlLayout->addLayout(endDateLayout);

    // Word frequency settings
    controlLayout->addWidget(new QLabel("Word Settings:"));

    auto minFreqLayout = new QHBoxLayout();
    minFreqLayout->addWidget(new QLabel("Min frequency:"));
    minWordFrequencySpinBox = new QSpinBox();
    minWordFrequencySpinBox->setRange(1, 10);
    minWordFrequencySpinBox->setValue(1);
    minFreqLayout->addWidget(minWordFrequencySpinBox);
    controlLayout->addLayout(minFreqLayout);

    auto maxWordsLayout = new QHBoxLayout();
    maxWordsLayout->addWidget(new QLabel("Max words:"));
    maxWordsSpinBox = new QSpinBox();
    maxWordsSpinBox->setRange(10, 200);
    maxWordsSpinBox->setValue(50);
    maxWordsLayout->addWidget(maxWordsSpinBox);
    controlLayout->addLayout(maxWordsLayout);

    // Generate button
    generateWordCloudButton = new QPushButton("Generate Word Cloud");
    generateWordCloudButton->
            setStyleSheet("background-color: #2196F3; color: white; padding: 10px; font-weight: bold;");
    controlLayout->addWidget(generateWordCloudButton);

    // Word count info
    wordCountLabel = new QLabel("Total words: 0");
    wordCountLabel->setStyleSheet("color: #666; padding: 5px;");
    controlLayout->addWidget(wordCountLabel);

    controlLayout->addStretch();

    // Right panel - word cloud
    auto cloudLayout = new QVBoxLayout();

    auto cloudTitle = new QLabel("Sleep Notes Word Cloud");
    cloudTitle->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    cloudLayout->addWidget(cloudTitle);

    wordCloudWidget = new WordCloudWidget();
    wordCloudWidget->setMinimumHeight(400);
    cloudLayout->addWidget(wordCloudWidget);

    auto infoLabel = new QLabel(
        "Tip: The word cloud shows the most frequently used words in your sleep notes. "
        "Larger words appear more often. Hover over words to see their frequency count.");
    infoLabel->setStyleSheet("color: #666; padding: 10px;");
    infoLabel->setWordWrap(true);
    cloudLayout->addWidget(infoLabel);

    // Add layouts to main
    mainLayout->addLayout(controlLayout, 1);
    mainLayout->addLayout(cloudLayout, 3);

    // Connect signals
    connect(generateWordCloudButton, &QPushButton::clicked, this, &MainWindow::loadWordCloudData);
    connect(wordCloudAllDateRangeCheckbox, &QCheckBox::toggled, [this](bool checked) {
        wordCloudStartDateEdit->setEnabled(!checked);
        wordCloudEndDateEdit->setEnabled(!checked);
    });
    connect(minWordFrequencySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        wordCloudWidget->setMinimumFrequency(value);
    });
    connect(maxWordsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        wordCloudWidget->setMaxWords(value);
    });
}

void MainWindow::showHistogramContextMenu(const QPoint &pos) {
    auto menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *resetZoomAction = menu->addAction("Reset Zoom");
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::resetHistogramZoom);

    menu->popup(histogramCustomPlot->mapToGlobal(pos));
}

void MainWindow::showAddSymptomDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Symptom");
    dialog.setModal(true);

    auto layout = new QVBoxLayout(&dialog);

    // Name
    layout->addWidget(new QLabel("Symptom Name:"));
    auto nameEdit = new QLineEdit();
    layout->addWidget(nameEdit);

    // Type
    layout->addWidget(new QLabel("Type:"));
    auto typeCombo = new QComboBox();
    typeCombo->addItem("Yes/No (Binary)", QVariant::fromValue(SymptomType::Binary));
    typeCombo->addItem("Count (e.g., times woke up)", QVariant::fromValue(SymptomType::Count));
    typeCombo->addItem("Quantity (e.g., drinks, mg)", QVariant::fromValue(SymptomType::Quantity));
    layout->addWidget(typeCombo);

    // Unit
    layout->addWidget(new QLabel("Unit (optional, for Count/Quantity):"));
    auto unitEdit = new QLineEdit();
    unitEdit->setPlaceholderText("e.g., times, drinks, mg, hours");
    layout->addWidget(unitEdit);

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    auto okButton = new QPushButton("Add");
    auto cancelButton = new QPushButton("Cancel");
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

void MainWindow::onExportHistoryToCSV() {
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Export History to CSV"),
                                                    QString("sleepbook_history_%1.csv").arg(
                                                        QDate::currentDate().toString("yyyy-MM-dd")),
                                                    tr("CSV Files (*.csv)"));

    if (!fileName.isEmpty()) {
        QList<QVariantMap> entries = loadSummaryData(); // This now uses loadAllEntries()
        exportHistoryToCSV(fileName, entries);
    }
}

void MainWindow::syncXAxes(const QCPRange &newRange) {
    auto senderAxis = qobject_cast<QCPAxis *>(sender());
    if (!senderAxis || !customPlot || !customPlot->plotLayout())
        return;

    bool oldSenderBlock = senderAxis->blockSignals(true);

    bool oldPlotBlock = customPlot->blockSignals(true);

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

    customPlot->blockSignals(oldPlotBlock);
    senderAxis->blockSignals(oldSenderBlock);

    customPlot->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::updateWindowTitle() {
    QString title = "Sleep and Health Logbook";
    if (UserManager::instance().isLoggedIn()) {
        title += QString(" - %1").arg(UserManager::instance().getCurrentUsername());
    }
    setWindowTitle(title);
}

bool MainWindow::updateSummaryEntry(const QUuid &entryId, const QDate &newDate, double duration,
                                    const QList<QPair<QString, double>> &symptomData) {
    QString symptomFile = getSymptomDataFile();
    QString password = UserManager::instance().getCurrentUser()->getEncryptionPassword();

    // Load existing summary entries
    QList<QVariantMap> entries;
    QByteArray existingData = DataEncryption::loadEncrypted(symptomFile, password);

    if (!existingData.isEmpty()) {
        QDataStream in(&existingData, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_5_15);
        in >> entries;
    }

    // Find and update the existing entry by ID
    bool entryFound = false;
    QString entryIdString = entryId.toString(QUuid::WithoutBraces);

    for (int i = 0; i < entries.size(); ++i) {
        QVariantMap &entry = entries[i];

        if (entry["id"].toString() == entryIdString) {
            // Update existing entry with new date and other data
            entry["date"] = newDate;
            entry["sleep_duration"] = duration;

            // Update symptom values
            for (const auto &pair : symptomData) {
                entry[pair.first] = pair.second;
            }

            entryFound = true;
            break;
        }
    }

    if (!entryFound) {
        qWarning() << "Entry not found in summary for ID:" << entryIdString;
        return false;
    }

    // Save updated entries back to summary file
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << entries;

    return DataEncryption::saveEncrypted(symptomFile, data, password);
}
