#include "todolistview.h"

#include "log.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QLabel>
#include <QScrollArea>
#include <QVariant>
#include <QComboBox>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QSizePolicy>

// --- Tools ---
#include "helper.h"

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
        // LOGI(TAG, "Adding timeblock: %s with %zu tasks.", tb.name, tb.tasks.size());

        // --- Container for label + list ---
        QWidget *column = new QWidget(this);
        QVBoxLayout *colLayout = new QVBoxLayout(column);
        colLayout->setContentsMargins(0, 0, 0, 0);
        colLayout->setSpacing(4);

        // --- Label and compact status drop down at top ---

        QWidget *titleRow = new QWidget(this);
        QHBoxLayout *titleRowLayout = new QHBoxLayout(titleRow);
        titleRowLayout->setContentsMargins(0, 0, 0, 0);
        titleRowLayout->setSpacing(6);

        QLabel *title = new QLabel(QString(tb.name), this);
        title->setStyleSheet("font-weight: bold; font-size: 18px;");
        // Make the title expand and center in the available space to the left
        // of the status icon. The status combobox below uses a fixed size.
        title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        title->setAlignment(Qt::AlignCenter);

        QComboBox *statusDrop = new QComboBox(this);
        // Small icon-only combo: Ongoing (blue), Stopped (red), Done (green)
        const QSize iconSize(12, 12);
        statusDrop->setIconSize(iconSize);
        statusDrop->setFixedWidth(30);
        statusDrop->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        statusDrop->setEditable(false);
        // Hide the standard drop-down arrow so the small colored icon isn't obscured.
        // This keeps the combo compact and visually an icon-only status marker.
        statusDrop->setStyleSheet(
            "QComboBox { border: none; background: transparent; padding: 0px; }"
            "QComboBox::down-arrow { image: none; }"
        );
        statusDrop->addItem(makeColorIcon(QColor(0, 122, 255), iconSize), "");
        statusDrop->addItem(makeColorIcon(QColor(204, 0, 0), iconSize), "");
        statusDrop->addItem(makeColorIcon(QColor(24, 160, 0), iconSize), "");

        // Map TimeblockStatus -> combo index
        int tbStatusIndex = 0;
        switch (tb.status)
        {
        case TimeblockStatus::ONGOING:
            tbStatusIndex = 0;
            break;
        case TimeblockStatus::STOPPED:
            tbStatusIndex = 1;
            break;
        case TimeblockStatus::DONE:
            tbStatusIndex = 2;
            break;
        }
        statusDrop->setCurrentIndex(tbStatusIndex);

        // When the user changes the small status dropdown, persist via repository.
        // Capture a copy of the timeblock (`tb`) so we can update its status and call repo.
        connect(statusDrop, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, tb](int idx)
                {
                    Timeblock newTb = tb;
                    // Map combo index to TimeblockStatus (same ordering)
                    if (idx == 0)
                        newTb.status = TimeblockStatus::ONGOING;
                    else if (idx == 1)
                        newTb.status = TimeblockStatus::STOPPED;
                    else
                        newTb.status = TimeblockStatus::DONE;

                    // Persist and refresh views
                    if (repo)
                        repo->updateTimeblock(newTb); });

        // Add title (expanding) first, then the fixed-size status widget to the right.
        statusDrop->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        titleRowLayout->addWidget(title);
        titleRowLayout->addWidget(statusDrop);

        colLayout->addWidget(titleRow);

        // --- Todo list ---
        QListWidget *list = new QListWidget(this);
        list->setSelectionMode(QAbstractItemView::SingleSelection);
        list->setMinimumWidth(300);
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
        // repo->habitCompletionPreview(const_cast<Task &>(task));
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