#include "log.h"

// --- GUI ---
#include "mainwindow.h"
// --- Data handling ---
#include "timeblock.h"
#include "database.h"

#include <QApplication> // UI app

int main(int argc, char *argv[])
{
    const char *TAG = "main";

    QApplication app(argc, argv);
    qRegisterMetaType<const Task *>("const Task*");

    // Initialize database and data repository
    CalendarRepository dbs;

    MainWindow window(nullptr, &dbs);
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
