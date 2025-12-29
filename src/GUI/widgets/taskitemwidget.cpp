#include "taskitemwidget.h"

#include <QVBoxLayout>
#include <QLabel>

TaskItemWidget::TaskItemWidget(const Task &t, QWidget *parent)
    : QWidget(parent)
{
    m_task = const_cast<Task *>(&t);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    QLabel *name = new QLabel(t.name);
    name->setStyleSheet("font-weight: bold; font-size: 14px;");

    QLabel *due = new QLabel(QString::fromStdString("Due: " + t.due_date_string()));

    // Priority
    // TODO: Replace with priority char
    QLabel *urgency = new QLabel("Priority: " + QString::fromStdString(t.priority_string()));

    // TODO: Add completed forum

    layout->addWidget(name);
    layout->addWidget(due);
    layout->addWidget(urgency);
}

Task *TaskItemWidget::task() const
{
    return m_task;
}