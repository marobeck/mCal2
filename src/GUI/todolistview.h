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
    void updateTasklist(const Timeblock &timeblock);
    void updateTasklists(const std::vector<Timeblock> &timeblocks);

signals:
    void taskSelected(const Task *task);
    void taskDeselected();

private slots:
    void onListCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    // Manage task completion toggles (receives Qt::CheckState values)
    void onTaskCompleted(const Task &task, int checkState);

private:
    CalendarRepository *repo = nullptr;

    QVector<QListWidget *> todoLists;
    QWidget *todoContainer;
    QHBoxLayout *todoLayout;
};