#pragma once
#include "task.h"

#include <QWidget>

#include <QLabel>
#include <QCheckBox>
#include <QString>
#include <QTimer>
class QResizeEvent;

// Forward declaration to avoid header include here
class CalendarRepository;

class TaskItemWidget : public QWidget
{
    Q_OBJECT
private:
    Task m_task;
    QString m_fullName;
    QLabel *m_nameLabel = nullptr;
    QCheckBox *m_doneCheck = nullptr;
    CalendarRepository *m_repo = nullptr;
    QLabel *m_priorityLabel = nullptr;

public:
    enum class Mode
    {
        FULL = 0,
        COMPACT = 1,
        PREVIEW = 2
    };
    Mode m_mode = Mode::FULL;

public:
    TaskItemWidget(const Task &t, CalendarRepository *repo = nullptr, QWidget *parent = nullptr, Mode mode = Mode::FULL);
    const Task &task() const; // Get associated task for reading

signals:
    void completionToggled(const Task &task, int checkState);

private slots:
    void onCompletionChanged(int checkState);

protected:
    void resizeEvent(QResizeEvent *event) override;
};
