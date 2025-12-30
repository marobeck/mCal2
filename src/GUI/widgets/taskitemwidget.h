#pragma once
#include "task.h"

#include <QWidget>

#include <QLabel>
#include <QCheckBox>
#
class TaskItemWidget : public QWidget
{
    Q_OBJECT
private:
    const Task &m_task;
    QLabel *m_nameLabel = nullptr;
    QCheckBox *m_doneCheck = nullptr;

public:
    TaskItemWidget(const Task &t, QWidget *parent = nullptr);
    const Task &task() const; // Get associated task for reading

signals:
    void completionToggled(const Task &task, bool completed);

private slots:
    void onCompletionChanged(bool checked);
};
