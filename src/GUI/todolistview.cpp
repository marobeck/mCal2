#include "todolistview.h"

#include "log.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QLabel>
#include <QScrollArea>
#include <QVariant>

// --- Widgets ---
#include "taskitemwidget.h"

TodoListView::TodoListView(QWidget *parent, CalendarRepository *dataRepo)
    : QWidget(parent), repo(dataRepo)
{
    const char *TAG = "TodoListView::Constructor";
    LOGI(TAG, "Initializing TodoListView...");

    if (!repo)
    {
        LOGE(TAG, "No CalendarRepository provided to TodoListView!");
        throw std::runtime_error("No CalendarRepository provided to TodoListView");
    }

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
    // Put the todo container inside the scroll area and add that to this
    // widget's root layout. Do NOT add `todoLayout` directly to the
    // root layout because `todoLayout` already has `todoContainer` as
    // its parent (which causes the "layout already has a parent"
    // warning).
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->addWidget(scrollArea, 3);

    setLayout(rootLayout);
}

void TodoListView::updateTasklists(const std::vector<Timeblock> &timeblocks)
{
    const char *TAG = "TodoListView::updateTasklists";
    LOGI(TAG, "Updating task lists with %zu timeblocks.", timeblocks.size());

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
        LOGI(TAG, "Adding timeblock: %s with %zu tasks.", tb.name, tb.tasks.size());

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
        list->setSelectionMode(QAbstractItemView::SingleSelection);
        list->setMinimumWidth(200);
        connect(list, &QListWidget::currentItemChanged, this, &TodoListView::onListCurrentItemChanged);

        for (auto &task : tb.tasks)
        {
            QListWidgetItem *item = new QListWidgetItem(list);
            TaskItemWidget *widget = new TaskItemWidget(task);

            item->setSizeHint(widget->sizeHint());
            list->addItem(item);
            list->setItemWidget(item, widget);
            repo->habitCompletionPreview(const_cast<Task &>(task));

            // --- Todo list widget handling ---
            connect(widget, &TaskItemWidget::completionToggled, this, &TodoListView::onTaskCompleted);
        }

        colLayout->addWidget(list);

        // Track list for your backend
        todoLists.append(list);

        // Add column to main horizontal layout
        todoLayout->addWidget(column);
    }
}

void TodoListView::onListCurrentItemChanged(QListWidgetItem *current, QListWidgetItem * /*previous*/)
{
    const char *TAG = "TodoListView::onListCurrentItemChanged";

    QListWidget *list = qobject_cast<QListWidget *>(sender());
    if (!list)
        return;

    if (!current)
    {
        emit taskDeselected();
        return;
    }

    // Deselect items in other lists to ensure only the newly-selected item
    // appears selected. Block their signals while doing this to avoid
    // recursive selection-change handling.
    for (QListWidget *other : todoLists)
    {
        if (other == list)
            continue;
        other->blockSignals(true);
        other->clearSelection();
        other->setCurrentItem(nullptr);
        other->blockSignals(false);
    }

    TaskItemWidget *w = qobject_cast<TaskItemWidget *>(list->itemWidget(current));

    emit taskSelected(&w->task());
    return;
}

void TodoListView::onTaskCompleted(const Task &task, int checkState)
{
    const char *TAG = "MainWindow::onTaskCompleted";

    if (task.status == TaskStatus::HABIT)
    {
        time_t now = time(nullptr);
        struct tm *tm_now = localtime(&now);
        LOGI(TAG, "Habit <%s> completion toggled with state %d on %04d-%02d-%02d", task.name,
             checkState, tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);

        if (checkState == Qt::Checked)
        {
            // For habits, we just log the completion date
            repo->addHabitEntry(task.uuid, now);
        }
        else if (checkState == Qt::Unchecked)
        {
            // For un-completing a habit, we remove today's entry
            repo->removeHabitEntry(task.uuid, now);
        }
        repo->habitCompletionPreview(const_cast<Task &>(task));
        return;
    }

    // For non-habits we accept three states: Unchecked, PartiallyChecked (in-progress), Checked
    const char *state_str = "unknown";
    if (checkState == Qt::Unchecked)
        state_str = "incomplete";
    else if (checkState == Qt::PartiallyChecked)
        state_str = "in-progress";
    else if (checkState == Qt::Checked)
        state_str = "completed";

    LOGI(TAG, "Task <%s> marked as %s (state %d)", task.name, state_str, checkState);

    // Update task status
    Task new_task = task;
    if (checkState == Qt::Unchecked)
        new_task.status = TaskStatus::INCOMPLETE;
    else if (checkState == Qt::PartiallyChecked)
        new_task.status = TaskStatus::IN_PROGRESS;
    else if (checkState == Qt::Checked)
        new_task.status = TaskStatus::COMPLETE;

    repo->updateTask(new_task);
}