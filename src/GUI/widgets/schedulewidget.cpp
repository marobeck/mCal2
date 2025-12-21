#include "schedulewidget.h"

#include <QPainter>

ScheduleWidget::ScheduleWidget(QWidget *parent)
    : QWidget(parent)
{
}

void ScheduleWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor(240, 240, 240));

    // Draw hourly markers
    int totalHours = 24;
    int h = height();
    int lineSpacing = h / totalHours;

    p.setPen(QPen(Qt::black, 1));

    for (int hour = 0; hour <= totalHours; hour++)
    {
        int y = hour * lineSpacing;
        p.drawLine(0, y, width(), y);
        p.drawText(5, y - 2, QString("%1:00").arg(hour, 2, 10, QChar('0')));
    }

    // Add timeblocks into the schedule
}