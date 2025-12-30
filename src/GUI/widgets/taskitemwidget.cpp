#include "taskitemwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFont>

TaskItemWidget::TaskItemWidget(const Task &t, QWidget *parent)
    : QWidget(parent), m_task(t)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // Top row: completion checkbox + name
    QHBoxLayout *topRow = new QHBoxLayout;
    m_doneCheck = new QCheckBox(this);
    bool completed = (m_task.status == TaskStatus::COMPLETE);
    m_doneCheck->setChecked(completed);
    connect(m_doneCheck, &QCheckBox::toggled, this, &TaskItemWidget::onCompletionChanged);

    m_nameLabel = new QLabel(m_task.name ? QString::fromUtf8(m_task.name) : QString("(untitled)"), this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(14);
    nameFont.setStrikeOut(completed);
    m_nameLabel->setFont(nameFont);

    topRow->addWidget(m_doneCheck, 0, Qt::AlignVCenter);
    topRow->addWidget(m_nameLabel, 1);
    layout->addLayout(topRow);

    // --- Due date *or* habit info ---
    // TODO: Add habit-specific UI elements

    // Due date
    QLabel *due = new QLabel(QString::fromStdString("Due: " + t.due_date_string()), this);
    layout->addWidget(due);

    // --- Priority ---
    QLabel *urgency = new QLabel("Priority: " + QString::fromStdString(t.priority_string()), this);
    layout->addWidget(urgency);
}

const Task &TaskItemWidget::task() const
{
    return m_task;
}

void TaskItemWidget::onCompletionChanged(bool checked)
{
    // Update visual strikeout
    QFont f = m_nameLabel->font();
    f.setStrikeOut(checked);
    m_nameLabel->setFont(f);

    emit completionToggled(m_task, checked);
}