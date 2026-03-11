#pragma once

#include <QWidget>
#include <QLabel>

class ScheduleWidget : public QWidget
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    ScheduleWidget(QWidget *parent = nullptr);
};