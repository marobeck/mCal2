/** overviewview.h
 * A panel for displaying highest urgency tasks for the day regardless of timeblock.
 * Displays a ledger of tasks completed today, and tracks number of tasks completed today.
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
    void updateOverview(const std::vector<Timeblock> &timeblocks);

private:
    CalendarRepository *repo = nullptr;

    QListWidget *m_urgentTasksList = nullptr;
};