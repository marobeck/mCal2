/**
 * mainwindow.cpp
 *
 * Overview of tasks, timeline, and projects.
 */

#include "mainwindow.h"
// Widgets
#include "taskitemwidget.h"

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
    dateTimeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss\ndddd, MMM d"));
}

void MainWindow::updateTasklists(const std::vector<Timeblock> &timeblocks)
{
    // Remove old widgets from layout
    // todo: Consider caching items to avoid constant redrawing
    while (QLayoutItem *item = todoLayout->takeAt(0))
    {
        QWidget *w = item->widget();
        if (w)
            w->deleteLater();
        delete item;
    }

    todoLists.clear();

    // Add lists to container instead of root directly
    for (const auto &tb : timeblocks)
    {

        // --- Container for label + list ---
        QWidget *column = new QWidget(this);
        QVBoxLayout *colLayout = new QVBoxLayout(column);
        colLayout->setContentsMargins(0, 0, 0, 0);
        colLayout->setSpacing(4);

        // --- Label at top ---
        QLabel *title = new QLabel(QString(tb.name), this);
        title->setAlignment(Qt::AlignHCenter);
        title->setStyleSheet("font-weight: bold; font-size: 16px;");
        colLayout->addWidget(title);

        // --- Todo list ---
        QListWidget *list = new QListWidget(this);

        for (const auto &task : tb.tasks)
        {
            QListWidgetItem *item = new QListWidgetItem(list);
            TaskItemWidget *widget = new TaskItemWidget(task);

            item->setSizeHint(widget->sizeHint());
            list->addItem(item);
            list->setItemWidget(item, widget);
        }

        colLayout->addWidget(list);

        // Track list for your backend
        todoLists.append(list);

        // Add column to main horizontal layout
        todoLayout->addWidget(column);
    }
}