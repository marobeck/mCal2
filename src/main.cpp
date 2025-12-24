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

    std::vector<Timeblock> Blocks;

    // Initialize database
    Database db;

    // Load timeblocks from database
    db.load_timeblocks(Blocks);
    for (auto &tb : Blocks)
    {
        db.load_tasks(&tb);
    }

    window.todoListView->updateTasklists(Blocks);

    return app.exec();
}
