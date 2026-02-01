/** helper.h
 * Helper functions for GUI components
 */

#pragma once
#include <QIcon>
#include <QColor>
#include <QSize>
#include <QDate>

// Helper to make a small circular colored icon
QIcon makeColorIcon(const QColor &c, const QSize &sz);

// Helper to get a date from the user via a dialog
QDate getDateFromUser(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QDate &defaultDate = QDate::currentDate());