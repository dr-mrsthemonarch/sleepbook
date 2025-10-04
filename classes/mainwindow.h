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
#include "symptom.h"
#include "symptomwidget.h"

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
    void rebuildSymptomWidgets();
    bool saveEntry(const QString& username);
    bool saveSummaryEntry(const QString& username);
    QString getCurrentDataDirectory();
    QString getSymptomDataFile();
    void showAddSymptomDialog();

    // UI Components
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

    QList<Symptom> symptoms;
    QList<SymptomWidget*> symptomWidgets;
};

#endif // MAINWINDOW_H