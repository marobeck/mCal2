#include "dayscheduleview.h"
#include <QPainter>
#include <QTime>
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>

// Widgets
#include "schedulewidget.h"

DayScheduleView::DayScheduleView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(300);

    dateTimeLabel = new QLabel("Loading...", this);
    dateTimeLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

    ScheduleWidget *scheduleWidget = new ScheduleWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(dateTimeLabel);
    layout->addWidget(scheduleWidget, 1);

    // Timer updates the clock once a second
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DayScheduleView::updateDateTime);
    timer->start(1000);

    updateDateTime();
}

void DayScheduleView::updateDateTime()
{
    dateTimeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss\ndddd, MMM d"));
}