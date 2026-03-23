#pragma once

#include <QWidget>
#include <QLabel>
#include "task.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QListWidget>
#include <QString>
#include "widgets/habitprogresswidget.h"

class EntryDetailsView : public QWidget
{
    Q_OBJECT
public:
    explicit EntryDetailsView(QWidget *parent = nullptr);
    void loadTask(const Task *task);
    // Populate habit completion dates (QDate) for the habit progress widget
    void setHabitCompletionDates(const QVector<QDate> &dates);

signals:
    void addHabitEntryRequested(const QString &taskUuid);
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
    // Prerequisite list container (compact-mode TaskItemWidgets)
    QLabel *m_prereqLabel = nullptr;
    QListWidget *m_prereqList = nullptr;
    // Dependency list container
    QLabel *m_dependentLabel = nullptr;
    QListWidget *m_dependentList = nullptr;
    // Habit functions
    QPushButton *m_addHabitEntryBtn = nullptr;
    HabitProgressWidget *m_habitProgress = nullptr;
    // Action buttons
    QPushButton *m_deleteBtn = nullptr;
    QPushButton *m_moveBtn = nullptr;
    QPushButton *m_editBtn = nullptr;

    // Current loaded task uuid (empty if none)
    QString m_currentTaskUuid;
};