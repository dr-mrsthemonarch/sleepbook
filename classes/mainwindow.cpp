#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    loadSymptoms();
}

MainWindow::~MainWindow() {
    saveSymptoms();
}

void MainWindow::setupUI() {
    setWindowTitle("Sleep Quality Tracker");
    resize(900, 600);
    
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left Panel - Notes
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QLabel* notesLabel = new QLabel("Sleep Notes:");
    notesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(notesLabel);
    
    // Date and time inputs
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("Date:"));
    dateEdit = new QDateEdit();
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateLayout->addWidget(dateEdit);
    dateLayout->addStretch();
    leftLayout->addLayout(dateLayout);
    
    QHBoxLayout* timeLayout = new QHBoxLayout();
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
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save Entry");
    saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px; font-weight: bold;");
    clearButton = new QPushButton("Clear");
    clearButton->setStyleSheet("padding: 8px;");
    
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(clearButton);
    leftLayout->addLayout(buttonLayout);
    
    // Right Panel - Symptoms
    QVBoxLayout* rightLayout = new QVBoxLayout();
    QLabel* symptomsLabel = new QLabel("Symptoms/Factors:");
    symptomsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(symptomsLabel);
    
    symptomsList = new QListWidget();
    rightLayout->addWidget(symptomsList);
    
    // Add new symptom
    QHBoxLayout* addSymptomLayout = new QHBoxLayout();
    newSymptomEdit = new QLineEdit();
    newSymptomEdit->setPlaceholderText("New symptom/factor");
    addSymptomButton = new QPushButton("Add");
    addSymptomLayout->addWidget(newSymptomEdit);
    addSymptomLayout->addWidget(addSymptomButton);
    rightLayout->addLayout(addSymptomLayout);
    
    // Add panels to main layout
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);
    
    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveEntry);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearForm);
    connect(addSymptomButton, &QPushButton::clicked, this, &MainWindow::onAddSymptom);
    connect(newSymptomEdit, &QLineEdit::returnPressed, this, &MainWindow::onAddSymptom);
    
    // Default symptoms
    defaultSymptoms << "Insomnia" << "Snoring" << "Nightmares" << "Restless"
                    << "Tired upon waking" << "Difficulty falling asleep"
                    << "Woke up during night" << "Sleep apnea symptoms"
                    << "Stress/Anxiety" << "Caffeine before bed"
                    << "Alcohol consumption" << "Exercise during day"
                    << "Screen time before bed" << "Room too hot/cold";
}

void MainWindow::loadSymptoms() {
    symptomsList->clear();
    
    QFile file("symptoms.txt");
    QStringList symptoms;
    
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                symptoms << line;
            }
        }
        file.close();
    }
    
    // Use default if file doesn't exist or is empty
    if (symptoms.isEmpty()) {
        symptoms = defaultSymptoms;
    }
    
    // Add as checkable items
    for (const QString& symptom : symptoms) {
        QListWidgetItem* item = new QListWidgetItem(symptom, symptomsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::saveSymptoms() {
    QFile file("symptoms.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (int i = 0; i < symptomsList->count(); ++i) {
            out << symptomsList->item(i)->text() << "\n";
        }
        file.close();
    }
}

void MainWindow::onAddSymptom() {
    QString newSymptom = newSymptomEdit->text().trimmed();
    if (!newSymptom.isEmpty()) {
        // Check if it already exists
        bool exists = false;
        for (int i = 0; i < symptomsList->count(); ++i) {
            if (symptomsList->item(i)->text() == newSymptom) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            QListWidgetItem* item = new QListWidgetItem(newSymptom, symptomsList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            newSymptomEdit->clear();
            saveSymptoms();
        } else {
            QMessageBox::information(this, "Duplicate", "This symptom already exists in the list.");
        }
    }
}

void MainWindow::onClearForm() {
    notesEdit->clear();
    dateEdit->setDate(QDate::currentDate());
    bedtimeEdit->setTime(QTime(22, 0));
    wakeupEdit->setTime(QTime(7, 0));
    
    for (int i = 0; i < symptomsList->count(); ++i) {
        symptomsList->item(i)->setCheckState(Qt::Unchecked);
    }
}

QString MainWindow::getCurrentDataDirectory() {
    // For now, using "default" user - we'll add login system next
    QString dataDir = "sleep_data/default";
    QDir dir;
    if (!dir.exists(dataDir)) {
        dir.mkpath(dataDir);
    }
    return dataDir;
}

bool MainWindow::saveEntry(const QString& username) {
    QString dataDir = getCurrentDataDirectory();
    
    // Create filename with date
    QString filename = QString("%1/sleep_%2.csv")
                          .arg(dataDir)
                          .arg(dateEdit->date().toString("yyyy-MM-dd"));
    
    // Check if file exists to determine if we need header
    bool fileExists = QFile::exists(filename);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not save entry!");
        return false;
    }
    
    QTextStream out(&file);
    
    // Write header if new file
    if (!fileExists) {
        out << "timestamp,date,bedtime,waketime,sleep_duration_hours,notes,symptoms\n";
    }
    
    // Calculate sleep duration
    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;
    
    // Collect checked symptoms
    QStringList checkedSymptoms;
    for (int i = 0; i < symptomsList->count(); ++i) {
        if (symptomsList->item(i)->checkState() == Qt::Checked) {
            checkedSymptoms << symptomsList->item(i)->text();
        }
    }
    
    // Prepare notes (escape quotes and newlines)
    QString notes = notesEdit->toPlainText();
    notes.replace("\"", "\"\"");
    notes.replace("\n", " ");
    
    // Write entry
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << ","
        << dateEdit->date().toString("yyyy-MM-dd") << ","
        << bedtimeEdit->time().toString("HH:mm") << ","
        << wakeupEdit->time().toString("HH:mm") << ","
        << QString::number(hours, 'f', 2) << ","
        << "\"" << notes << "\","
        << "\"" << checkedSymptoms.join(";") << "\"\n";
    
    file.close();
    return true;
}

void MainWindow::onSaveEntry() {
    if (saveEntry("default")) {
        QMessageBox::information(this, "Success", "Sleep entry saved successfully!");
        onClearForm();
    }
}