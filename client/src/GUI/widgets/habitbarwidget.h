#pragma once

#include "task.h"
#include <QWidget>

class HabitBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HabitBarWidget(QWidget *parent = nullptr);

    void setValues(const TaskStatus *values);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const TaskStatus *m_values;
};
