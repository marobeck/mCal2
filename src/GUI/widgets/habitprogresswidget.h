#pragma once

#include <QWidget>
#include <QVector>
#include <QDate>

class HabitProgressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HabitProgressWidget(QWidget *parent = nullptr);
    // Provide a list of completion dates (time points) covering the past year
    void setCompletions(const QVector<QDate> &dates);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // completions contains dates on which the habit was completed
    QVector<QDate> m_completions;
};
