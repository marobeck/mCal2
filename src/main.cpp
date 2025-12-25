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

    MainWindow window;
    window.resize(1200, 800);
    window.show();

    /* -------------------------------- Test data ------------------------------- */

    // Initialize database
    Database db;

    // Load timeblocks from database
    db.load_timeblocks(window.timeblocks);
    for (auto &tb : window.timeblocks)
    {
        db.load_tasks(&tb);
    }

    window.todoListView->updateTasklists(window.timeblocks);
    window.newEntryView->populateTimeblocks(window.timeblocks);

    return app.exec();
}
