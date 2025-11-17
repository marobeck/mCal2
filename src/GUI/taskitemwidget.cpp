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

    // todo: Implement task functions to neatly display said information
    //  QLabel *due = new QLabel("Due: " + t.due_date_string());
    //  QLabel *urgancy = new QLabel("Priority: " + QString::number(t.get_urgancy()));

    layout->addWidget(name);
    // layout->addWidget(due);
    // layout->addWidget(urgancy);
}
