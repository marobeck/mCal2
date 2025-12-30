#pragma once

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVector>

#include "timeblock.h"
#include "calendarrepository.h"

class TodoListView : public QWidget
{
    Q_OBJECT
public:
    explicit TodoListView(QWidget *parent = nullptr, CalendarRepository *dataRepo = nullptr);
    void updateTasklists(const std::vector<Timeblock> &timeblocks);

signals:
    void taskSelected(const Task *task);
    void taskDeselected();

private slots:
    void onListCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    // Manage task completion toggles
    void onTaskCompleted(const Task &task, bool completed);

private:
    CalendarRepository *repo = nullptr;

    QVector<QListWidget *> todoLists;
    QWidget *todoContainer;
    QHBoxLayout *todoLayout;
};