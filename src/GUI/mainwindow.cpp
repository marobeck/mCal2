/**
 * mainwindow.cpp
 *
 * Overview of tasks, timeline, and projects.
 */

#include "mainwindow.h"

// Qt functions
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // --- Left side: Date/time + schedule ---
    dateTimeLabel = new QLabel("Loading...", this);
    dateTimeLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

    scheduleView = new DayScheduleView(this);

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(dateTimeLabel);
    leftLayout->addWidget(scheduleView, 1); // stretch factor for resize handling

    // --- Right side: todo lists ---
    // Container for all todo lists
    todoContainer = new QWidget(this);
    todoLayout = new QHBoxLayout(todoContainer);
    todoLayout->setContentsMargins(0, 0, 0, 0);
    todoLayout->setSpacing(10);

    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(todoContainer);

    // Final placement beside the schedule
    QHBoxLayout *rootLayout = new QHBoxLayout;
    rootLayout->addLayout(leftLayout, 2);
    rootLayout->addWidget(scrollArea, 3);

    central->setLayout(rootLayout);

    central->setLayout(rootLayout);

    // Timer updates the clock once a second
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000);

    updateDateTime();
}

void MainWindow::updateDateTime()
{
    dateTimeLabel->setText(QDateTime::currentDateTime().toString("dddd, MMM d  hh:mm:ss"));
}

void MainWindow::updateTasklists(const std::vector<Timeblock> &timeblocks)
{
    // Remove old widgets from layout
    while (QLayoutItem *item = todoLayout->takeAt(0))
    {
        QWidget *w = item->widget();
        if (w)
            w->deleteLater();
        delete item;
    }

    todoLists.clear();

    // Add lists to container instead of root directly
    for (size_t i = 0; i < timeblocks.size(); i++)
    {
        QListWidget *list = new QListWidget(this);
        list->addItem(QString(timeblocks[i].name));

        // Print names per task
        for (size_t j = 0; j < timeblocks[i].tasks.size(); j++)
        {
            list->addItem(QString(timeblocks[i].tasks[j].name));
        }

        // Keep track for backend use
        todoLists.append(list);

        todoLayout->addWidget(list);
    }
}