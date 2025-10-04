#include "mainwindow.h"
#include <QToolBar>
#include <QDataStream>
#include <QBuffer>
#include "logindialog.h"

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

    for (const Symptom& s : symptoms) {
        out << s.getName() << Symptom::typeToString(s.getType()) << s.getUnit();
    }

    DataEncryption::saveEncrypted(userSymptomsFile, data, password);
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
    QString username = "default";

    if (UserManager::instance().isLoggedIn()) {
        User* user = UserManager::instance().getCurrentUser();
        if (user) {
            username = user->getUsername();
        }
    }

    QString dataDir = QString("sleep_data/%1").arg(username);
    QDir dir;
    if (!dir.exists(dataDir)) {
        dir.mkpath(dataDir);
    }
    return dataDir;
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
    QList<QPair<QString, double>> symptomData;
    for (SymptomWidget* widget : symptomWidgets) {
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
    for (SymptomWidget* widget : symptomWidgets) {
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
    }
}