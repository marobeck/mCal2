#include "taskitemwidget.h"

#include <QVBoxLayout>
#include <QLabel>

TaskItemWidget::TaskItemWidget(const Task &t, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    QLabel *name = new QLabel(t.name);
    name->setStyleSheet("font-weight: bold; font-size: 14px;");

    QLabel *due = new QLabel(QString::fromStdString("Due: " + t.due_date_string()));
    QLabel *urgency = new QLabel("Urgency: " + QString::number(t.get_urgency()));

    layout->addWidget(name);
    layout->addWidget(due);
    layout->addWidget(urgency);
}
