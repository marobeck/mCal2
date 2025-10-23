#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QVector>
#include <QListWidget>

#include "dayscheduleview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void updateDateTime();

private:
    QLabel *dateTimeLabel;
    DayScheduleView *scheduleView;
    QVector<QListWidget *> todoLists;
};