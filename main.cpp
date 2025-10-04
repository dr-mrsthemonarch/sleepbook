//created by drmrsthemonarch with ai effort
#include "classes/mainwindow.h"
#include "classes/logindialog.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Show login dialog
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0; // User cancelled login
    }

    // Show main window if login successful
    MainWindow window;
    window.show();

    return app.exec();
}
