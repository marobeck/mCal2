#pragma once
#include "task.h"

#include <QWidget>

#include <QLabel>
#include <QCheckBox>
#include <QString>
class QResizeEvent;

class TaskItemWidget : public QWidget
{
    Q_OBJECT
private:
    const Task &m_task;
    QString m_fullName;
    QLabel *m_nameLabel = nullptr;
    QCheckBox *m_doneCheck = nullptr;

public:
    TaskItemWidget(const Task &t, QWidget *parent = nullptr);
    const Task &task() const; // Get associated task for reading

signals:
    void completionToggled(const Task &task, int checkState);

private slots:
    void onCompletionChanged(int checkState);
protected:
    void resizeEvent(QResizeEvent *event) override;
};
