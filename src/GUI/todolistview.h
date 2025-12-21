#pragma once

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVector>

#include "timeblock.h"

class TodoListView : public QWidget
{
    Q_OBJECT
public:
    explicit TodoListView(QWidget *parent = nullptr);
    void updateTasklists(const std::vector<Timeblock> &timeblocks);

signals:
    void taskSelected(Task *task);
    void taskDeselected();

private slots:
    void onListCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    QVector<QListWidget *> todoLists;
    QWidget *todoContainer;
    QHBoxLayout *todoLayout;

};