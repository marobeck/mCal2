/** overviewview.h
 * A panel for displaying highest urgency tasks for the day regardless of timeblock.
 * Displays a ledger of tasks completed today, and tracks number of tasks completed today.
 * TODO: Add indicator for ongoing timeblocks and their top tasks, so user can see at a glance if they have any ongoing timeblocks and what they are.
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVector>

#include "calendarrepository.h"
#include "task.h"

class OverviewView : public QWidget
{
    Q_OBJECT
public:
    explicit OverviewView(QWidget *parent = nullptr, CalendarRepository *dataRepo = nullptr);
    void updateOverview();

private:
    CalendarRepository *repo = nullptr;

    QListWidget *m_urgentTasksList = nullptr;
    QListWidget *m_completedTasksList = nullptr;
};