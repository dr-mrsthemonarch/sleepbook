#include "mainwindow.h"
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    createUserToolbar();
    loadSymptoms();
    rebuildSymptomWidgets();
    updateWindowTitle();
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
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);

    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveEntry);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearForm);
    connect(addSymptomButton, &QPushButton::clicked, this, &MainWindow::onAddSymptom);
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
        User* user = UserManager::instance().getCurrentUser();
        QString displayText = user->getDisplayName().isEmpty() ?
                             user->getUsername() : user->getDisplayName();
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
        UserManager::instance().logout();
        close(); // Close main window, will show login dialog again in main.cpp
    }
}

void MainWindow::loadSymptoms() {
    symptoms.clear();

    QFile file("symptoms.txt");

    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                Symptom s = Symptom::deserialize(line);
                symptoms.append(s);
            }
        }
        file.close();
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
    QFile file("symptoms.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const Symptom& s : symptoms) {
            out << s.serialize() << "\n";
        }
        file.close();
    }
}

void MainWindow::rebuildSymptomWidgets() {
    // Clear existing widgets
    qDeleteAll(symptomWidgets);
    symptomWidgets.clear();

    // Create new widgets
    for (const Symptom& s : symptoms) {
        SymptomWidget* widget = new SymptomWidget(s);
        symptomWidgets.append(widget);
        symptomsLayout->addWidget(widget);
    }

    symptomsLayout->addStretch();
}

void MainWindow::showAddSymptomDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Symptom");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Name
    layout->addWidget(new QLabel("Symptom Name:"));
    QLineEdit* nameEdit = new QLineEdit();
    layout->addWidget(nameEdit);

    // Type
    layout->addWidget(new QLabel("Type:"));
    QComboBox* typeCombo = new QComboBox();
    typeCombo->addItem("Yes/No (Binary)", QVariant::fromValue(SymptomType::Binary));
    typeCombo->addItem("Count (e.g., times woke up)", QVariant::fromValue(SymptomType::Count));
    typeCombo->addItem("Quantity (e.g., drinks, mg)", QVariant::fromValue(SymptomType::Quantity));
    layout->addWidget(typeCombo);

    // Unit
    layout->addWidget(new QLabel("Unit (optional, for Count/Quantity):"));
    QLineEdit* unitEdit = new QLineEdit();
    unitEdit->setPlaceholderText("e.g., times, drinks, mg, hours");
    layout->addWidget(unitEdit);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("Add");
    QPushButton* cancelButton = new QPushButton("Cancel");
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
            for (const Symptom& s : symptoms) {
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

    for (SymptomWidget* widget : symptomWidgets) {
        widget->reset();
    }
}

QString MainWindow::getCurrentDataDirectory() {
    QString dataDir = "sleep_data/default";
    QDir dir;
    if (!dir.exists(dataDir)) {
        dir.mkpath(dataDir);
    }
    return dataDir;
}

QString MainWindow::getSymptomDataFile() {
    QString dataDir = getCurrentDataDirectory();
    return QString("%1/symptom_history.csv").arg(dataDir);
}

bool MainWindow::saveEntry(const QString& username) {
    if (!UserManager::instance().isLoggedIn()) {
        QMessageBox::warning(this, "Not Logged In", "Please login to save entries.");
        return false;
    }

    QString dataDir = getCurrentDataDirectory();

    QString filename = QString("%1/sleep_%2.csv")
                          .arg(dataDir)
                          .arg(dateEdit->date().toString("yyyy-MM-dd"));

    bool fileExists = QFile::exists(filename);

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not save entry!");
        return false;
    }

    QTextStream out(&file);

    if (!fileExists) {
        out << "timestamp,date,bedtime,waketime,sleep_duration_hours,notes,symptoms\n";
    }

    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

    QStringList symptomStrings;
    for (SymptomWidget* widget : symptomWidgets) {
        Symptom s = widget->getSymptom();
        if (s.isPresent()) {
            if (s.getType() == SymptomType::Binary) {
                symptomStrings << s.getName();
            } else {
                symptomStrings << QString("%1:%2%3")
                    .arg(s.getName())
                    .arg(s.getValue())
                    .arg(s.getUnit().isEmpty() ? "" : " " + s.getUnit());
            }
        }
    }

    QString notes = notesEdit->toPlainText();
    notes.replace("\"", "\"\"");
    notes.replace("\n", " ");

    out << QDateTime::currentDateTime().toString(Qt::ISODate) << ","
        << dateEdit->date().toString("yyyy-MM-dd") << ","
        << bedtimeEdit->time().toString("HH:mm") << ","
        << wakeupEdit->time().toString("HH:mm") << ","
        << QString::number(hours, 'f', 2) << ","
        << "\"" << notes << "\","
        << "\"" << symptomStrings.join("; ") << "\"\n";

    file.close();

    return saveSummaryEntry(username);
}

bool MainWindow::saveSummaryEntry(const QString& username) {
    QString symptomFile = getSymptomDataFile();
    bool fileExists = QFile::exists(symptomFile);

    QFile file(symptomFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    if (!fileExists) {
        out << "date,sleep_duration_hours";
        for (const Symptom& s : symptoms) {
            out << "," << s.getColumnName();
        }
        out << "\n";
    }

    QDateTime bedDateTime(dateEdit->date(), bedtimeEdit->time());
    QDateTime wakeDateTime(dateEdit->date().addDays(1), wakeupEdit->time());
    if (wakeupEdit->time() > bedtimeEdit->time()) {
        wakeDateTime = QDateTime(dateEdit->date(), wakeupEdit->time());
    }
    double hours = bedDateTime.secsTo(wakeDateTime) / 3600.0;

    out << dateEdit->date().toString("yyyy-MM-dd") << ","
        << QString::number(hours, 'f', 2);

    for (SymptomWidget* widget : symptomWidgets) {
        Symptom s = widget->getSymptom();
        out << "," << QString::number(s.getValue(), 'f', 1);
    }
    out << "\n";
    
    file.close();
    return true;
}

void MainWindow::onSaveEntry() {
    if (saveEntry("default")) {
        QMessageBox::information(this, "Success", "Sleep entry saved successfully!");
        onClearForm();
    }
}