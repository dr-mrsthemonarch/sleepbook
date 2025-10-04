#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QListWidget>
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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSaveEntry();
    void onAddSymptom();
    void onClearForm();

private:
    void setupUI();
    void loadSymptoms();
    void saveSymptoms();
    bool saveEntry(const QString& username);
    QString getCurrentDataDirectory();

    // UI Components
    QTextEdit* notesEdit;
    QListWidget* symptomsList;
    QDateEdit* dateEdit;
    QTimeEdit* bedtimeEdit;
    QTimeEdit* wakeupEdit;
    QLineEdit* newSymptomEdit;
    QPushButton* saveButton;
    QPushButton* clearButton;
    QPushButton* addSymptomButton;

    QStringList defaultSymptoms;
};

#endif // MAINWINDOW_H