#include "taskitemwidget.h"
#include "habitbarwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFont>
#include <QFontMetrics>
#include <QTimer>
#include <QSizePolicy>
#include <QListWidget>
#include <QAbstractScrollArea>

#include "log.h"

#include "calendarrepository.h"
#include <ctime>

TaskItemWidget::TaskItemWidget(const Task &t, CalendarRepository *repo, QWidget *parent, Mode mode)
    : QWidget(parent), m_task(t), m_repo(repo), m_mode(mode)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // --- Top row: completion checkbox + name ---
    QHBoxLayout *topRow = new QHBoxLayout;

    // Check box
    m_doneCheck = new QCheckBox(this);
    bool completed = false;
    // If it's a task (not habit), set tristate based on status
    if (m_task.status != TaskStatus::HABIT)
    {
        m_doneCheck->setTristate(true);
        if (m_task.status == TaskStatus::INCOMPLETE)
        {
            m_doneCheck->setCheckState(Qt::Unchecked);
        }
        else if (m_task.status == TaskStatus::IN_PROGRESS)
        {
            m_doneCheck->setCheckState(Qt::PartiallyChecked);
        }
        else if (m_task.status == TaskStatus::COMPLETE)
        {
            m_doneCheck->setCheckState(Qt::Checked);
            completed = true;
        }
    }
    else
    {
        // For habits, just use binary checked/unchecked
        completed = (m_task.completed_days[0] == TaskStatus::COMPLETE);
        m_doneCheck->setCheckState(completed ? Qt::Checked : Qt::Unchecked);
    }

    // Use stateChanged(int) so we can handle tristate for tasks and binary for habits
    connect(m_doneCheck, &QCheckBox::stateChanged, this, &TaskItemWidget::onCompletionChanged);

    m_fullName = m_task.name ? QString::fromUtf8(m_task.name) : QString("(untitled)");
    m_nameLabel = new QLabel(m_fullName, this);
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setToolTip(m_fullName);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(12);
    nameFont.setStrikeOut(completed);
    m_nameLabel->setFont(nameFont);

    topRow->addWidget(m_doneCheck, 0, Qt::AlignVCenter);
    topRow->addWidget(m_nameLabel, 1);

    if (mode == Mode::COMPACT)
    {
        // Display priority in a single character for compact mode
        char priorityChar = t.priority_char();
        if (priorityChar != ' ')
        {
            m_priorityLabel = new QLabel(QString(priorityChar), this);
            QFont priorityFont = m_priorityLabel->font();
            priorityFont.setBold(true);
            priorityFont.setPointSize(12);
            m_priorityLabel->setFont(priorityFont);
            m_priorityLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            topRow->addWidget(m_priorityLabel, 0, Qt::AlignRight);
        }
    }

    layout->addLayout(topRow);

    if (mode == Mode::COMPACT)
    {
        // Ignore rest of details in compact mode
        resizeEvent(nullptr); // Trigger initial eliding of name
        return;
    }

    // --- Due date *or* habit info ---
    if (t.status == TaskStatus::HABIT)
    {
        HabitBarWidget *habitInfo = new HabitBarWidget(this);
        habitInfo->setValues(t.completed_days);
        layout->addWidget(habitInfo);
    }
    else
    {
        // Due date
        QLabel *due = new QLabel(QString::fromStdString("Due: " + t.due_date_string()), this);
        layout->addWidget(due);
    }

    // --- Priority + Scope ---
    QLabel *urgency = new QLabel("Priority: " + QString::fromStdString(t.priority_string()) + ", Scope: " + QString::fromStdString(t.scope_string()), this);
    layout->addWidget(urgency);
}

const Task &TaskItemWidget::task() const
{
    return m_task;
}

void TaskItemWidget::onCompletionChanged(int checkState)
{
    // Update visual strikeout: only fully checked items are struck out
    QFont f = m_nameLabel->font();
    f.setStrikeOut(checkState == Qt::Checked);
    m_nameLabel->setFont(f);

    emit completionToggled(m_task, checkState);

    // If we have a repository pointer, handle persistence directly from here
    if (m_repo)
    {
        // For habits, add/remove today's entry asynchronously
        if (m_task.status == TaskStatus::HABIT)
        {
            time_t now = time(nullptr);
            if (checkState == Qt::Checked)
            {
                Task tcopy = m_task;
                QTimer::singleShot(0, this, [this, tcopy, now]()
                                   {
                    if (m_repo)
                        m_repo->addHabitEntry(tcopy.uuid, now); });
            }
            else if (checkState == Qt::Unchecked)
            {
                Task tcopy = m_task;
                QTimer::singleShot(0, this, [this, tcopy, now]()
                                   {
                    if (m_repo)
                        m_repo->removeHabitEntry(tcopy.uuid, now); });
            }
            return;
        }

        // For non-habits, update task status and persist asynchronously
        Task new_task = m_task;
        if (checkState == Qt::Unchecked)
            new_task.status = TaskStatus::INCOMPLETE;
        else if (checkState == Qt::PartiallyChecked)
            new_task.status = TaskStatus::IN_PROGRESS;
        else if (checkState == Qt::Checked)
            new_task.status = TaskStatus::COMPLETE;

        QTimer::singleShot(0, this, [this, new_task]()
                           {
            if (m_repo)
                m_repo->updateTask(new_task); });
    }
}

void TaskItemWidget::resizeEvent(QResizeEvent *event)
{
    // QWidget::resizeEvent(event);

    // Only do eliding behavior in compact mode
    if (m_mode != Mode::COMPACT)
        return;

    if (!m_nameLabel)
        return;

    QFontMetrics fm(m_nameLabel->font());

    // Prefer the containing list/viewport width when available to get consistent
    // eliding behavior with the list view, otherwise fall back own width
    int total = 0;
    QWidget *p = this->parentWidget();
    while (p)
    {
        // If parent is a QListWidget, use its viewport width
        if (auto *lw = qobject_cast<QListWidget *>(p))
        {
            total = lw->viewport()->width();
            break;
        }
        // If parent is a QAbstractScrollArea (like a list view), use its viewport
        if (auto *asa = qobject_cast<QAbstractScrollArea *>(p))
        {
            total = asa->viewport()->width();
            break;
        }
        p = p->parentWidget();
    }
    if (total == 0)
        total = this->width();

    int used = 0;
    const int padding = 16; // left/right margins from layout
    const int spacing = 8;  // spacing between widgets

    if (m_doneCheck)
        used += m_doneCheck->sizeHint().width() + spacing;

    if (m_priorityLabel)
        used += m_priorityLabel->sizeHint().width() + spacing;

    int avail = total - used - padding;
    if (avail < 20)
        avail = 20;

    QString el = fm.elidedText(m_fullName, Qt::ElideRight, avail);
    // Only update the label text if it actually changed to avoid triggering
    // another layout pass unnecessarily.
    if (m_nameLabel->text() != el)
    {
        m_nameLabel->setText(el);
        // LOGI("TaskItemWidget::resizeEvent", "Resizing TaskItemWidget: full name '%s', available width %d, elided name '%s'",
        //      m_fullName.toUtf8().constData(), avail, el.toUtf8().constData());
    }
}