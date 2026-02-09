#include "habitprogresswidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDate>

HabitProgressWidget::HabitProgressWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
}

void HabitProgressWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Ensure we repaint when size changes
    update();
}

void HabitProgressWidget::setCompletions(const QVector<QDate> &dates)
{
    m_completions = dates;
    update(); // trigger repaint
}

void HabitProgressWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw a simple year grid similar to GitHub contributions: 52 columns (weeks) x 7 rows (days)
    const int maxWeeks = 52;
    const int minCellSize = 10; // minimum desired cell size in pixels
    const int days = 7;

    const int padding = 3;
    const int gridLeft = padding;
    const int gridTop = padding;
    const int gridWidth = width() - padding * 2;
    const int gridHeight = qMin((height() - 60), 7 * 12); // leave room for bars below

    // Determine number of weeks to display so cell width >= minCellSize
    int candidateWeeks = qMax(1, gridWidth / (minCellSize + 2));
    int weeks = qMin(maxWeeks, candidateWeeks);
    if (weeks < 4)
        weeks = qMin(maxWeeks, 4); // keep a reasonable minimum view

    int cellW = qMax(minCellSize, (gridWidth / weeks) - 2);
    int cellH = cellW; // keep cells square

    // Build a set of completion dates for quick lookup
    // QSet<QDate> doneSet = QSet<QDate>::fromList(m_completions.toList());
    QSet<QDate> doneSet = QSet<QDate>(m_completions.begin(), m_completions.end());

    // Compute start date: roughly (weeks-1) weeks back from today,
    // then align to the previous Sunday so rows map: 0=Sunday .. 6=Saturday
    QDate today = QDate::currentDate();
    QDate approx = today.addDays(-((weeks - 1) * 7));
    // QDate::dayOfWeek() returns 1=Mon .. 7=Sun. Step back to Sunday.
    int dow = approx.dayOfWeek();
    int daysToPrevSunday = dow % 7; // Sunday -> 0, Monday -> 1, ..., Saturday -> 6
    QDate start = approx.addDays(-daysToPrevSunday);

    for (int w = 0; w < weeks; ++w)
    {
        for (int d = 0; d < days; ++d)
        {
            QDate day = start.addDays(w * 7 + d);
            int x = gridLeft + w * (cellW + 2);
            int y = gridTop + d * (cellH + 2);

            // Don't draw cells that are in the future relative to today
            if (day > today)
                continue;

            QColor color = QColor(235, 237, 240, 100); // empty
            if (doneSet.contains(day))
                color = QColor(38, 166, 91); // green

            p.setBrush(color);
            p.setPen(Qt::NoPen);
            p.drawRect(x, y, cellW, cellH);
        }
    }

    /*
    // --- Draw this months completion bar ---
    // Compute month ranges for the last 12 months
    int barTop = gridTop + days * (cellH + 2) + 12;
    int barHeight = 20;
    int monthBarLeft = padding;
    int monthBarWidth = width() - padding;

    QDate mo = today.addDays(1 - today.day()); // first day of the month
    QFontMetrics fm(font());

    int daysInMonth = mo.daysInMonth();
    int completed = 0;
    for (int d = 0; d < daysInMonth; ++d)
    {
        QDate dd = mo.addDays(d);
        if (doneSet.contains(dd))
            ++completed;
    }

    float pct = daysInMonth ? (float)completed / (float)daysInMonth : 0.0f;

    int segW = monthBarWidth / 12;
    int sx = monthBarLeft * segW;

    // Background
    p.setBrush(QColor(230, 230, 230));
    p.setPen(Qt::NoPen);
    p.drawRect(sx, barTop, segW - 4, barHeight);

    // Filled portion
    p.setBrush(QColor(100, 181, 246));
    int fillW = qMax(1, static_cast<int>((segW - 4) * pct));
    p.drawRect(sx, barTop, fillW, barHeight);

    // Draw percentage text above each bar
    QString pctStr = QString::number(static_cast<int>(pct * 100)) + "%";
    p.setPen(Qt::black);
    p.drawText(sx, barTop - 2, segW, 12, Qt::AlignHCenter | Qt::AlignBottom, pctStr);
    */
}
