/**
 * newentryview.h
 * GUI view for creating a new task entry
 */
#pragma once

#include "task.h"
#include "database.h"

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <vector>

class Timeblock;

class NewEntryView : public QWidget
{
    Q_OBJECT
public:
    explicit NewEntryView(QWidget *parent = nullptr);

    // Populate the timeblock dropdown with pointers (caller owns objects)
    void populateTimeblocks(const std::vector<Timeblock> &timeblocks);
signals:
    // Emitted when the user creates a new task. Caller receives ownership of the Task*
    void taskCreated(Task *task, int timeblockIndex);

private slots:
    void onTypeChanged(int index);
    void onHabitModeChanged(int index);
    void onCreateClicked();

private:
    // Widgets
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_descEdit = nullptr;
    QComboBox *m_timeblockCombo = nullptr;
    QComboBox *m_priorityCombo = nullptr;
    QComboBox *m_typeCombo = nullptr; // Task or Habit
    QDateTimeEdit *m_dueEdit = nullptr;

    // Habit controls
    QComboBox *m_habitModeCombo = nullptr; // Frequency or Weekday
    QSpinBox *m_frequencySpin = nullptr;   // 0-127
    QCheckBox *m_weekdayChecks[7];

    QPushButton *m_createBtn = nullptr;
};
