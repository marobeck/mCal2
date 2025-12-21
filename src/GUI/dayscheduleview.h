#pragma once
#include <QWidget>
#include <QLabel>

class DayScheduleView : public QWidget
{
    Q_OBJECT
private:
    QLabel *dateTimeLabel;

private slots:
    void updateDateTime();

public:
    explicit DayScheduleView(QWidget *parent = nullptr);
};
