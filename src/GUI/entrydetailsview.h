#pragma once

#include <QWidget>
#include <QLabel>
#include "task.h"

class EntryDetailsView : public QWidget
{
    Q_OBJECT
public:
    explicit EntryDetailsView(QWidget *parent = nullptr);
    void loadTask(Task *task);

private:
    // Persistent widgets that are created once and updated by loadTask
    QLabel *m_nameLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_dueLabel = nullptr;
    QLabel *m_priorityLabel = nullptr;
    QLabel *m_urgencyLabel = nullptr;
    QLabel *m_prereqLabel = nullptr;
};