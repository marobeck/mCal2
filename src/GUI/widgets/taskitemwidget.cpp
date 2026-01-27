#include "taskitemwidget.h"
#include "habitbarwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFont>
#include <QResizeEvent>
#include <QFontMetrics>

TaskItemWidget::TaskItemWidget(const Task &t, QWidget *parent)
    : QWidget(parent), m_task(t)
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
    m_nameLabel->setToolTip(m_fullName);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(12);
    nameFont.setStrikeOut(completed);
    m_nameLabel->setFont(nameFont);

    topRow->addWidget(m_doneCheck, 0, Qt::AlignVCenter);
    topRow->addWidget(m_nameLabel, 1);
    layout->addLayout(topRow);

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

    // --- Priority ---
    QLabel *urgency = new QLabel("Priority: " + QString::fromStdString(t.priority_string()), this);
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
}

void TaskItemWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Compute available width for the name label: widget width minus checkbox and margins
    int totalWidth = this->width();
    int checkboxWidth = m_doneCheck ? m_doneCheck->sizeHint().width() : 24;
    // Account for layout contents margins (set to 8 left + 8 right in constructor) and spacing
    const int horizMargins = 16; // left+right
    const int extraPadding = 80; // small buffer for spacing

    // Available width for name label
    int avail = totalWidth - checkboxWidth - horizMargins - extraPadding;
    if (avail < 20)
        avail = 20;

    QFontMetrics fm(m_nameLabel->font());
    QString elided = fm.elidedText(m_fullName, Qt::ElideRight, avail);
    m_nameLabel->setText(elided);
}