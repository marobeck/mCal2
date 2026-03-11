#pragma once

#include "timeblock.h"

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>

class NewTimeblockView : public QWidget
{
    Q_OBJECT
public:
    explicit NewTimeblockView(QWidget *parent = nullptr);

signals:
    // Emitted when the user creates a new timeblock. Caller receives ownership of the Timeblock*
    void timeblockCreated(Timeblock *timeblock);

public slots:
    void onCreateClicked();

private:
    // Widgets
    QLineEdit *m_nameEdit = nullptr;
    QTextEdit *m_descEdit = nullptr;

    QPushButton *m_createBtn = nullptr;
};