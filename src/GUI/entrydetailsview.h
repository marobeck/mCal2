#pragma once

#include <QWidget>
#include <QLabel>
#include "task.h"

#include <QPushButton>
#include <QString>

class EntryDetailsView : public QWidget
{
    Q_OBJECT
public:
    explicit EntryDetailsView(QWidget *parent = nullptr);
    void loadTask(const Task *task);

signals:
    void deleteTaskRequested(const QString &taskUuid);
    void moveTaskRequested(const QString &taskUuid);
    void editTaskRequested(const QString &taskUuid);

private:
    // Persistent widgets that are created once and updated by loadTask
    QLabel *m_nameLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_dueLabel = nullptr;
    QLabel *m_priorityLabel = nullptr;
    QLabel *m_urgencyLabel = nullptr;
    QLabel *m_prereqLabel = nullptr;
    // Action buttons
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_moveBtn = nullptr;
    QPushButton *m_editBtn = nullptr;

    // Current loaded task uuid (empty if none)
    QString m_currentTaskUuid;
};