#include "log.h"

#include "habitbarwidget.h"
#include <QPainter>
#include <QDate>

HabitBarWidget::HabitBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(30);
}

void HabitBarWidget::setValues(const TaskStatus *values)
{
    const char *TAG = "HabitBarWidget::setValues";

    m_values = values;
    update(); // trigger repaint
}

void HabitBarWidget::paintEvent(QPaintEvent *)
{
    if (!m_values)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const int n = 7;
    const int cellWidth = width() / n;
    const int cellHeight = height();

    painter.setPen(Qt::white);
    for (int i = 0; i < n; ++i)
    {
        const int spacing = 1;
        QRect rect(i * cellWidth + spacing,
                   spacing,
                   cellWidth - spacing * 2,
                   cellHeight - spacing * 2);

        if (m_values[i] == TaskStatus::COMPLETE)
        {
            painter.fillRect(rect, Qt::green);
            painter.setPen(Qt::darkBlue);
        }
        else if (m_values[i] == TaskStatus::IN_PROGRESS)
        {
            painter.fillRect(rect, Qt::blue);
            painter.setPen(Qt::white);
        }
        else // INCOMPLETE
        {
            painter.fillRect(rect, Qt::lightGray);
            painter.setPen(Qt::black);
        }

        // --- Draw days of week labels ---

        // Assume m_values[0] is today, m_values[1] is yesterday, etc. so label from right to left
        int dayOfWeek = QDate::currentDate().dayOfWeek();

        painter.setFont(QFont("Arial", 8));
        QString dayLabel = QDate::shortDayName((dayOfWeek + 6 - i) % 7 + 1); // 1=Mon, 7=Sun
        painter.drawText(rect, Qt::AlignCenter, dayLabel);
    }
}
