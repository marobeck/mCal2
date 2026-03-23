#include "entrydetailsview.h"

#include "log.h"

#include <QVBoxLayout>
#include <QString>
#include <QHBoxLayout>
#include <QPushButton>
#include "widgets/habitprogresswidget.h"
#include "widgets/taskitemwidget.h"

EntryDetailsView::EntryDetailsView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    m_nameLabel = new QLabel("", this);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(m_nameLabel);

    m_dueLabel = new QLabel("", this);
    layout->addWidget(m_dueLabel);

    m_priorityLabel = new QLabel("", this);
    layout->addWidget(m_priorityLabel);

    m_urgencyLabel = new QLabel("", this);
    layout->addWidget(m_urgencyLabel);

    m_descLabel = new QLabel("", this);
    m_descLabel->setWordWrap(true);
    layout->addWidget(m_descLabel);

    // Prerequisite list container (compact-mode TaskItemWidgets)
    m_prereqLabel = new QLabel("Prerequisites:", this);
    m_prereqList = new QListWidget(this);
    m_prereqList->setStyleSheet("QListWidget { background: transparent; border: none; }");
    m_prereqList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_prereqLabel->setVisible(false); // hidden until prerequisites exist
    m_prereqList->setVisible(false);  // hidden until prerequisites exist
    layout->addWidget(m_prereqLabel);
    layout->addWidget(m_prereqList);

    // Prerequisite list container (compact-mode TaskItemWidgets)
    m_dependentLabel = new QLabel("Dependencies:", this);
    m_dependentList = new QListWidget(this);
    m_dependentList->setStyleSheet("QListWidget { background: transparent; border: none; }");
    m_dependentList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_dependentLabel->setVisible(false);
    m_dependentList->setVisible(false);
    layout->addWidget(m_dependentLabel);
    layout->addWidget(m_dependentList);

    // Habit progress widget (created but hidden until a habit is loaded)
    m_habitProgress = new HabitProgressWidget(this);
    m_habitProgress->setVisible(false);
    layout->addWidget(m_habitProgress);

    // Force everything to the top
    layout->addStretch();

    m_addHabitEntryBtn = new QPushButton("Add Habit Entry", this);
    layout->addWidget(m_addHabitEntryBtn);

    connect(m_addHabitEntryBtn, &QPushButton::clicked, this, [this]()
            { if (!m_currentTaskUuid.isEmpty())
                emit addHabitEntryRequested(m_currentTaskUuid); });

    // --- Action buttons at bottom ---
    QHBoxLayout *btnRow = new QHBoxLayout;
    m_deleteBtn = new QPushButton("Delete", this);
    m_moveBtn = new QPushButton("Move", this);
    m_editBtn = new QPushButton("Edit", this);

    // Default disabled until a task is loaded
    m_deleteBtn->setEnabled(false);
    m_moveBtn->setEnabled(false);
    m_editBtn->setEnabled(false);

    btnRow->addWidget(m_deleteBtn);
    btnRow->addWidget(m_moveBtn);
    btnRow->addWidget(m_editBtn);
    layout->addLayout(btnRow);

    connect(m_deleteBtn, &QPushButton::clicked, this, [this]()
            {
        if (!m_currentTaskUuid.isEmpty())
            emit deleteTaskRequested(m_currentTaskUuid); });
    connect(m_moveBtn, &QPushButton::clicked, this, [this]()
            {
        if (!m_currentTaskUuid.isEmpty())
            emit moveTaskRequested(m_currentTaskUuid); });
    connect(m_editBtn, &QPushButton::clicked, this, [this]()
            {
        if (!m_currentTaskUuid.isEmpty())
            emit editTaskRequested(m_currentTaskUuid); });
}

void EntryDetailsView::loadTask(const Task *task)
{
    const char *TAG = "EntryDetailsView::loadTask";

    LOGI(TAG, "Loading task details view for task \"%s\" (%p)", task->name, task);

    if (!task)
    {
        LOGW(TAG, "Null task provided to loadTask; clearing view.");

        // Clear all fields
        m_nameLabel->setText("");
        m_descLabel->setText("");
        m_dueLabel->setText("");
        m_priorityLabel->setText("");
        m_urgencyLabel->setText("");
        m_currentTaskUuid.clear();
        m_deleteBtn->setEnabled(false);
        m_moveBtn->setEnabled(false);
        m_editBtn->setEnabled(false);
        return;
    }

    // Name (may be null)
    QString name = task->name ? QString::fromUtf8(task->name) : QString("(untitled)");
    m_nameLabel->setText(name);

    // Description (may be null)
    QString desc = task->desc ? QString::fromUtf8(task->desc) : QString("");
    m_descLabel->setText(desc);

    // Due date string via Task API
    try
    {
        std::string due = task->due_date_full_string();
        m_dueLabel->setText(QString::fromStdString("Due: " + due));
    }
    catch (...)
    {
        m_dueLabel->setText("Due: (n/a)");
    }

    // Priority
    std::string priorityStr;
    try
    {
        priorityStr = task->priority_string();
        m_priorityLabel->setText(QString::fromStdString("Priority: " + priorityStr));
    }
    catch (...)
    {
        m_priorityLabel->setText("Priority: (n/a)");
    }

    // Urgency via Task API
    float urg = 0.0f;
    try
    {
        urg = task->get_urgency();
        m_urgencyLabel->setText(QString::fromStdString("Urgency: " + std::to_string(urg)));
    }
    catch (...)
    {
        m_urgencyLabel->setText("Urgency: (n/a)");
    }

    // Prerequisites: show compact widgets for each prereq task (if any)
    // Update the list widget with the top tasks
    m_prereqList->clear();

    bool hasPrereqs = false;
    if (!task->prerequisites.empty())
    {
        for (Task *p : task->prerequisites)
        {
            if (!p)
            {
                LOGW(TAG, "Null prereq found, skipping");
                continue;
            }
            QListWidgetItem *item = new QListWidgetItem(m_prereqList);
            // Display item in preview mode (no modifications can be made from here, so we don't need repo reference)
            TaskItemWidget *widget = new TaskItemWidget(p, nullptr, this, TaskItemWidget::Mode::PREVIEW);
            item->setSizeHint(widget->sizeHint());
            m_prereqList->addItem(item);
            m_prereqList->setItemWidget(item, widget);
        }
        hasPrereqs = true;
    }

    m_prereqLabel->setVisible(hasPrereqs);
    m_prereqList->setVisible(hasPrereqs);

    // Dependencies: show compact widgets for each prereq task (if any)
    // Update the list widget with the top tasks
    m_dependentList->clear();

    bool hasDependent = false;
    if (!task->dependents.empty())
    {
        for (Task *d : task->dependents)
        {
            if (!d)
                continue;
            QListWidgetItem *item = new QListWidgetItem(m_dependentList);
            // Display item in preview mode (no modifications can be made from here, so we don't need repo reference)
            TaskItemWidget *widget = new TaskItemWidget(d, nullptr, this, TaskItemWidget::Mode::PREVIEW);
            item->setSizeHint(widget->sizeHint());
            m_dependentList->addItem(item);
            m_dependentList->setItemWidget(item, widget);
        }
        hasDependent = true;
    }

    m_dependentLabel->setVisible(hasDependent);
    m_dependentList->setVisible(hasDependent);

    // Habit data
    if (task->status == TaskStatus::HABIT)
    {
        m_addHabitEntryBtn->setEnabled(true);
        m_addHabitEntryBtn->setVisible(true);
        // Show habit progress area; actual dates will be populated by MainWindow
        m_habitProgress->setVisible(true);
    }
    else
    {
        m_addHabitEntryBtn->setVisible(false);
        m_addHabitEntryBtn->setEnabled(false);
        m_habitProgress->setVisible(false);
    }

    // Track the currently-loaded task and enable actions
    m_currentTaskUuid = task->uuid;
    m_deleteBtn->setEnabled(true);
    m_moveBtn->setEnabled(true);
    m_editBtn->setEnabled(true);
}

void EntryDetailsView::setHabitCompletionDates(const QVector<QDate> &dates)
{
    if (m_habitProgress)
    {
        m_habitProgress->setCompletions(dates);
        m_habitProgress->setVisible(!dates.isEmpty());
    }
}