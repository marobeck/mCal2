/**
 * newentryview.h
 * GUI view for creating a new task entry
 */
#pragma once

#include "task.h"
#include "database.h"

#include <QLabel>
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
    // Load a task for editing
    void loadTaskForEditing(Task *task);
    // Clear all input fields and reset to new entry mode
    void clearFields();
signals:
    // Emitted when the user creates a new task. Caller receives ownership of the Task*
    void taskCreated(Task *task, int timeblockIndex);
    // Emitted when the user edits an existing task. Caller receives ownership of the Task*
    void taskEdited(Task *task);

private slots:
    void onTypeChanged(int index);
    void onCreateClicked();

private:
    // Edit mode
    bool m_editMode = false;
    const Task *m_editingTask = nullptr; // Pointer to task being edited, DOES NOT OWN

    // Widgets
    QLabel *m_title = nullptr;

    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_descEdit = nullptr;
    QComboBox *m_timeblockCombo = nullptr;
    QComboBox *m_priorityCombo = nullptr;
    QComboBox *m_typeCombo = nullptr; // Task or Habit
    QDateTimeEdit *m_dueEdit = nullptr;
    QCheckBox *m_undatedCheck = nullptr;

    // Widgets grouped under completion parameters
    QWidget *m_dueRowWidget = nullptr;
    QWidget *m_freqWidget = nullptr;
    QWidget *m_weekdayWidget = nullptr;
    // Habit controls
    QSpinBox *m_frequencySpin = nullptr; // 0-127
    QCheckBox *m_weekdayChecks[7];

    QPushButton *m_createBtn = nullptr;
};
