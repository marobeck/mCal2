#pragma once
#include <QWidget>

class DayScheduleView : public QWidget
{
    Q_OBJECT
public:
    explicit DayScheduleView(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
