/**
 * mainwindow.cpp
 * 
 * Overview of tasks, timeline, and projects.
 */

#include "mainwindow.h"
#include <QTimer>
#include <QDateTime>
#include <QHBoxLayout>
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
    QWidget *todoContainer = new QWidget(this);
    QHBoxLayout *todoLayout = new QHBoxLayout(todoContainer);
    todoLayout->setContentsMargins(0, 0, 0, 0);
    todoLayout->setSpacing(10);

    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(todoContainer);

    // Add lists to container instead of root directly
    int numberOfLists = 10; // Now unlimited lists possible!
    for (int i = 0; i < numberOfLists; ++i)
    {
        QListWidget *list = new QListWidget(this);
        list->addItem(QString("Task List %1").arg(i + 1));
        list->addItem(QString("======================================================================"));
        list->addItem(QString("wawa"));

        // Keep track for backend use
        todoLists.append(list);

        todoLayout->addWidget(list);
    }

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
