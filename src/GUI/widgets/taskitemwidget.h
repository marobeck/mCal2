#pragma once
#include "task.h"

#include <QWidget>

class TaskItemWidget : public QWidget
{
    Q_OBJECT
private:
    Task *m_task = nullptr;

public:
    TaskItemWidget(const Task &t, QWidget *parent = nullptr);
    Task *task() const;
};
