#include <QApplication>
#include <QFont>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // App-wide font
    QFont font("Segoe UI", 10);
    app.setFont(font);

    MainWindow w;
    w.show();

    return app.exec();
}
