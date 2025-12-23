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

TodoListView::TodoListView(QWidget *parent)
    : QWidget(parent)
{
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
    if (w && w->task())
    {
        emit taskSelected(w->task());
        return;
    }

    LOGE(TAG, "Failed to get TaskItemWidget from current item.");
    
    // Fallback: if the Task* was stored in the item data
    QVariant v = current->data(Qt::UserRole);
    if (v.isValid())
    {
        qulonglong raw = v.toULongLong();
        Task *t = reinterpret_cast<Task *>(raw);
        if (t)
            emit taskSelected(t);
    }
}
