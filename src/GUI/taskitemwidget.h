#pragma once
#include "task.h"

#include <QWidget>

class TaskItemWidget : public QWidget
{
    Q_OBJECT
public:
    TaskItemWidget(const Task &t, QWidget *parent = nullptr);
};
