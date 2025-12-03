// DEBUG
#include <iostream>

// --- GUI ---
#include "mainwindow.h"
// --- Data handling ---
#include "timeblock.h"

#include <QApplication> // UI app

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1200, 800);
    window.show();

    std::vector<Timeblock> Blocks;

    // Pet a dog every day at 13:00 for ~20 minutes
    {
        Timeblock block1("Dog time", "Go meet a dog", 0x7F, (time_t)1000, (time_t)46800);

        Task task1("Pet dog", "Pet a dog on the head", Priority::MEDIUM, 1764829922);
        Task task2("See dog", "Witness a creature", Priority::VERY_HIGH);

        block1.append(task1);
        block1.append(task2);

        std::cout << task1.get_urgancy() << std::endl;
        Blocks.push_back(block1);
    }
    {
        Timeblock block1("Sleep time", "Please sleep", 0x7F, (time_t)28800, (time_t)82800);

        Blocks.push_back(block1);
    }
    {
        Timeblock block1("Work time", "Get paid", 0x7F, (time_t)28800, (time_t)82800);

        Task task1("Eat evidence", "", Priority::VERY_HIGH, 1764729922);
        block1.append(task1);

        Blocks.push_back(block1);
    }

    window.updateTasklists(Blocks);

    return app.exec();
}
