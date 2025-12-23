/**
 * mainwindow.cpp
 *
 * Manages the main application window and its scenes.
 */

#include "mainwindow.h"

#include "log.h"

// Qt functions
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollArea>
#include <QStackedWidget>

// Widgets
#include "taskitemwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // --- Left side scene manager ---
    leftStack = new QStackedWidget(this);
    scheduleView = new DayScheduleView(this);
    newEntryView = new NewEntryView(this);
    entryDetailsView = new EntryDetailsView(this);

    // Add schedule + detail/edit views to left stack
    leftStack->addWidget(scheduleView);     // index 0
    leftStack->addWidget(entryDetailsView); // index 1
    leftStack->addWidget(newEntryView);     // index 2

    leftStack->setCurrentWidget(scheduleView);

    // --- Right side scene manager ---
    rightStack = new QStackedWidget(this);

    todoListView = new TodoListView(this);

    // Add to stack
    rightStack->addWidget(todoListView); // index 0

    // Initialize the right panel to the todo list
    rightStack->setCurrentWidget(todoListView);

    // Layout: left scene stack on the left, scenes on the right
    QHBoxLayout *root = new QHBoxLayout;
    root->addWidget(leftStack, 2);
    root->addWidget(rightStack, 3);

    central->setLayout(root);

    // --- Todo list task selection handling ---
    connect(todoListView, &TodoListView::taskSelected, this, [this](Task *t)
            {
        // When a task is selected in the todo list, show its details on the right panel
        QVariant v = QVariant::fromValue(static_cast<void*>(t));
        switchLeftPanel(Scene::EntryDetails, v); });
    connect(todoListView, &TodoListView::taskDeselected, this, [this]()
            {
        // When no task is selected, switch back to schedule view
        switchLeftPanel(Scene::DaySchedule); });
}

void MainWindow::switchRightPanel(Scene scene, QVariant data)
{
    const char *TAG = "MainWindow::switchRightPanel";

    switch (scene)
    {
    case Scene::TodoList:
        rightStack->setCurrentWidget(todoListView);
        currentRightScene = Scene::TodoList;
        break;
    }

    LOGI(TAG, "Switched right panel to scene %d", static_cast<int>(currentRightScene));
}

void MainWindow::switchLeftPanel(Scene scene, QVariant data)
{
    const char *TAG = "MainWindow::switchLeftPanel";

    switch (scene)
    {
    case Scene::DaySchedule:
        leftStack->setCurrentWidget(scheduleView);
        currentLeftScene = Scene::DaySchedule;
        break;

    case Scene::NewEntry:
        // If a Task pointer was supplied, load it into the details view
        if (data.canConvert<void *>())
        {
            Task *t = static_cast<Task *>(data.value<void *>());
            entryDetailsView->loadTask(t);
        }
        // Show the details/edit view on the left (use EntryDetailsView to
        // display task fields when switching from the todo list).
        leftStack->setCurrentWidget(entryDetailsView);
        currentLeftScene = Scene::NewEntry;
        break;

    case Scene::EntryDetails:
        if (data.canConvert<void *>())
        {
            Task *t = static_cast<Task *>(data.value<void *>());
            entryDetailsView->loadTask(t);
        }
        leftStack->setCurrentWidget(entryDetailsView);
        currentLeftScene = Scene::EntryDetails;
        break;

    default:
        leftStack->setCurrentWidget(scheduleView);
        currentLeftScene = Scene::DaySchedule;
        break;
    }

    LOGI(TAG, "Switched left panel to scene %d", static_cast<int>(currentLeftScene));
}
