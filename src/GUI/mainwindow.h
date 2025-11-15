#pragma once
#include "timeblock.h"
#include "dayscheduleview.h"

#include <vector>
// --- UI ---
#include <QMainWindow>
#include <QLabel>
#include <QVector>
#include <QListWidget>
#include <QHBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void updateTasklists(const std::vector<Timeblock> &timeblocks);

private slots:
    void updateDateTime();

private:
    QLabel *dateTimeLabel;
    DayScheduleView *scheduleView;
    QVector<QListWidget *> todoLists;
    QWidget *todoContainer;
    QHBoxLayout *todoLayout;
};