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
#include <QPushButton>
#include <QIcon>
#include <QSize>
#include <QSpacerItem>
#include <QMessageBox>
#include <QInputDialog>
#include <cstring>
#include <string>

/* -------------------------------------------------------------------------- */
/*                                     GUI                                    */
/* -------------------------------------------------------------------------- */

MainWindow::MainWindow(QWidget *parent, CalendarRepository *dataPtr)
    : QMainWindow(parent), repo(dataPtr)
{
    const char *TAG = "MainWindow::Constructor";
    LOGI(TAG, "Initializing main window...");

    if (!repo)
    {
        LOGE(TAG, "No CalendarRepository provided to MainWindow!");
        throw std::runtime_error("No CalendarRepository provided to MainWindow");
    }

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    // --- Left / Right thin edge panels (icon buttons) ---
    leftEdgePanel = new QWidget(this);
    leftEdgePanel->setFixedWidth(42);
    leftEdgeLayout = new QVBoxLayout(leftEdgePanel);
    leftEdgeLayout->setContentsMargins(4, 4, 4, 4);
    leftEdgeLayout->setSpacing(6);

    rightEdgePanel = new QWidget(this);
    rightEdgePanel->setFixedWidth(42);
    rightEdgeLayout = new QVBoxLayout(rightEdgePanel);
    rightEdgeLayout->setContentsMargins(4, 4, 4, 4);
    rightEdgeLayout->setSpacing(6);

    // --- Left side scene manager ---
    leftStack = new QStackedWidget(this);
    scheduleView = new DayScheduleView(this);
    newEntryView = new NewEntryView(this);
    newTimeblockView = new NewTimeblockView(this);
    entryDetailsView = new EntryDetailsView(this);

    // Add schedule + detail/edit views to left stack
    leftStack->addWidget(scheduleView);     // index 0
    leftStack->addWidget(entryDetailsView); // index 1
    leftStack->addWidget(newEntryView);     // index 2
    leftStack->addWidget(newTimeblockView); // index 3

    leftStack->setCurrentWidget(scheduleView);

    // --- Right side scene manager ---
    rightStack = new QStackedWidget(this);

    todoListView = new TodoListView(this, repo);

    // Add to stack
    rightStack->addWidget(todoListView); // index 0

    // Initialize the right panel to the todo list
    rightStack->setCurrentWidget(todoListView);

    // Layout: thin left edge panel, left scene stack, right scene stack, thin right edge
    QHBoxLayout *root = new QHBoxLayout;
    root->addWidget(leftEdgePanel, 0);
    root->addWidget(leftStack, 2);
    root->addWidget(rightStack, 3);
    root->addWidget(rightEdgePanel, 0);

    central->setLayout(root);

    // --- Edge icons ---
    addLeftEdgeButton(QIcon("../icons/schedule.png"), Scene::DaySchedule);
    addLeftEdgeButton(QIcon("../icons/new_timeblock.png"), Scene::NewTimeblock);
    addLeftEdgeButton(QIcon("../icons/new_entry.png"), Scene::NewEntry);

    addRightEdgeButton(QIcon("../icons/todo_list.png"), Scene::TodoList);

    // Force everything to the top
    leftEdgeLayout->addStretch();
    rightEdgeLayout->addStretch();

    // Update the views with the current model
    modelChanged();

    /* -------------------------------------------------------------------------- */
    /*                              Signal management                             */
    /* -------------------------------------------------------------------------- */

    // --- Todo list task selection handling ---
    connect(todoListView, &TodoListView::taskSelected, this, [this](const Task *t)
            {
        // When a task is selected in the todo list, show its details on the right panel
        Q_ASSERT(t);    // Enforce non-null task pointer
        QVariant v = QVariant::fromValue(t);
        switchLeftPanel(Scene::EntryDetails, v); });
    connect(todoListView, &TodoListView::taskDeselected, this, [this]()
            {
        // When no task is selected, switch back to schedule view
        switchLeftPanel(Scene::DaySchedule); });

    // --- New entry handling ---
    connect(newEntryView, &NewEntryView::taskCreated, this, &MainWindow::onTaskCreated);
    connect(newEntryView, &NewEntryView::taskEdited, this, &MainWindow::onTaskEdited);
    connect(newTimeblockView, &NewTimeblockView::timeblockCreated, this, &MainWindow::onTimeblockCreated);

    connect(repo, &CalendarRepository::modelChanged, this, &MainWindow::modelChanged);

    // EntryDetailsView actions
    connect(entryDetailsView, &EntryDetailsView::deleteTaskRequested, this, &MainWindow::onDeleteTaskRequested);
    connect(entryDetailsView, &EntryDetailsView::moveTaskRequested, this, &MainWindow::onMoveTaskRequested);
    connect(entryDetailsView, &EntryDetailsView::editTaskRequested, this, &MainWindow::onEditTaskRequested);
}

void MainWindow::onDeleteTaskRequested(const QString &taskUuid)
{
    const char *TAG = "MainWindow::onDeleteTaskRequested";
    LOGI(TAG, "Delete requested for task %s", taskUuid.toUtf8().constData());

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Task",
                                                              "Are you sure you want to delete this task?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    // Call repository to remove task
    if (!repo->removeTask(taskUuid.toUtf8().constData()))
    {
        LOGE(TAG, "Failed to remove task %s", taskUuid.toUtf8().constData());
    }
}

void MainWindow::onMoveTaskRequested(const QString &taskUuid)
{
    const char *TAG = "MainWindow::onMoveTaskRequested";
    LOGI(TAG, "Move requested for task %s", taskUuid.toUtf8().constData());

    // Build list of timeblock names
    const auto &tbs = repo->timeblocks();
    if (tbs.empty())
    {
        QMessageBox::information(this, "Move Task", "No timeblocks available to move to.");
        return;
    }

    QStringList items;
    for (const auto &tb : tbs)
        items << QString(tb.name);

    bool ok = false;
    QString chosen = QInputDialog::getItem(this, "Move Task", "Select destination timeblock:", items, 0, false, &ok);
    if (!ok || chosen.isEmpty())
        return;

    // Find selected timeblock UUID
    const char *destUuid = nullptr;
    std::string cname = chosen.toStdString();
    for (const auto &tb : tbs)
    {
        if (cname == tb.name)
        {
            destUuid = tb.uuid;
            break;
        }
    }
    if (!destUuid)
    {
        LOGE(TAG, "Selected timeblock not found: %s", chosen.toUtf8().constData());
        return;
    }

    if (!repo->moveTask(taskUuid.toUtf8().constData(), destUuid))
    {
        LOGE(TAG, "Failed to move task %s to %s", taskUuid.toUtf8().constData(), destUuid);
    }
}

void MainWindow::onEditTaskRequested(const QString &taskUuid)
{
    const char *TAG = "MainWindow::onEditTaskRequested";
    LOGI(TAG, "Edit requested for task %s", taskUuid.toUtf8().constData());

    /** How to edit a task:
     * Pass the task UUID to NewEntryView by value, note that the UUID will be the same.
     * NewEntryView will populate its fields with the task data, and will be in edit mode.
     * When NewEntryView saves the task call repo->updateTask() to overwrite the existing task.
     */

    // Pass UUID to new entry view
    QVariant v = QVariant::fromValue(taskUuid);
    switchLeftPanel(Scene::NewEntry, v);
}

void MainWindow::addLeftEdgeButton(const QIcon &icon, Scene scene, QVariant data)
{
    QPushButton *btn = new QPushButton(leftEdgePanel);
    btn->setIcon(icon);
    btn->setIconSize(QSize(24, 24));
    btn->setFlat(true);
    btn->setFixedSize(36, 36);
    leftEdgeLayout->addWidget(btn);
    // connect click to switchLeftPanel
    connect(btn, &QPushButton::clicked, this, [this, scene, data]()
            { switchLeftPanel(scene, data); });
}

void MainWindow::addRightEdgeButton(const QIcon &icon, Scene scene, QVariant data)
{
    QPushButton *btn = new QPushButton(rightEdgePanel);
    btn->setIcon(icon);
    btn->setIconSize(QSize(24, 24));
    btn->setFlat(true);
    btn->setFixedSize(36, 36);
    rightEdgeLayout->addWidget(btn);
    // connect click to switchRightPanel
    connect(btn, &QPushButton::clicked, this, [this, scene, data]()
            { switchRightPanel(scene, data); });
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
        // If a Task UUID was supplied, load it into the details view
        if (data.canConvert<QString>())
        {
            QString uuid = data.toString();
            newEntryView->loadTaskForEditing(repo->findTaskByUuid(uuid.toUtf8().constData()));
        }
        else
        {
            if (data.isValid())
            {
                LOGW(TAG, "Invalid data provided to NewEntry scene; expected Task* or null.");
            }
            newEntryView->clearFields();
        }
        newEntryView->populateTimeblocks(repo->timeblocks());

        // Show the details/edit view on the left (use EntryDetailsView to
        // display task fields when switching from the todo list).
        leftStack->setCurrentWidget(newEntryView);
        currentLeftScene = Scene::NewEntry;
        break;

    case Scene::NewTimeblock:
        leftStack->setCurrentWidget(newTimeblockView);
        currentLeftScene = Scene::NewTimeblock;
        break;

    case Scene::EntryDetails:
        if (data.canConvert<const Task *>())
        {
            const Task *t = data.value<const Task *>();
            Q_ASSERT(t);
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

/* -------------------------------------------------------------------------- */
/*                                   Runtime                                  */
/* -------------------------------------------------------------------------- */

void MainWindow::onTaskCreated(Task *task, int timeblockIndex)
{
    const char *TAG = "MainWindow::onTaskCreated";

    LOGI(TAG, "Task <%s> created for timeblock index %d", task->name, timeblockIndex);

    repo->addTask(*task, timeblockIndex);

    // We copied the Task into the Timeblock, so free the heap allocation
    delete task;
}

void MainWindow::onTaskEdited(Task *task)
{
    const char *TAG = "MainWindow::onTaskEdited";

    LOGI(TAG, "Task <%s> edited", task->name);

    if (!repo->updateTask(*task))
    {
        LOGE(TAG, "Failed to update task <%s>", task->name);
    }

    // We copied the Task into the Timeblock, so free the heap allocation
    delete task;
}

void MainWindow::onTimeblockCreated(Timeblock *timeblock)
{
    const char *TAG = "MainWindow::onTimeblockCreated";

    LOGI(TAG, "Timeblock <%s> created", timeblock->name);

    repo->addTimeblock(*timeblock);

    // Free the heap allocation
    delete timeblock;
}

void MainWindow::modelChanged()
{
    const char *TAG = "MainWindow::modelChanged";
    LOGI(TAG, "Calendar model has changed; updating views...");

    // Update tasklist order
    repo->sortTimeblocks();
    // Refresh the TodoListView with the new model snapshot
    todoListView->updateTasklists(repo->timeblocks());
    // Also update NewEntryView's timeblock list
    newEntryView->populateTimeblocks(repo->timeblocks());
}