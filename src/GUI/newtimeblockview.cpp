#include "newtimeblockview.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>

NewTimeblockView::NewTimeblockView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Title
    QLabel *title = new QLabel("New Timeblock", this);
    title->setStyleSheet("font-weight: bold; font-size: 18px;");
    layout->addWidget(title);

    // --- Form ---
    QFormLayout *form = new QFormLayout;

    // Name input
    m_nameEdit = new QLineEdit(this);
    form->addRow("Name:", m_nameEdit);

    // Description input
    m_descEdit = new QTextEdit(this);
    m_descEdit->setFixedHeight(80);
    form->addRow("Description:", m_descEdit);

    // TODO: Add parameters for repeating and single session timeblocks

    layout->addLayout(form);

    // Force everything to the top
    layout->addStretch();

    // Create button
    m_createBtn = new QPushButton("Create", this);
    connect(m_createBtn, &QPushButton::clicked, this, &NewTimeblockView::onCreateClicked);
    layout->addWidget(m_createBtn, 0, Qt::AlignRight);
}

void NewTimeblockView::onCreateClicked()
{
    // Build Timeblock object
    Timeblock *tb = new Timeblock();

    // Set name and description
    QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty())
        tb->name = strdup(name.toUtf8().constData());
    else
        tb->name = strdup("(untitled)");

    QString desc = m_descEdit->toPlainText();
    tb->desc = strdup(desc.toUtf8().constData());

    // Emit signal
    emit timeblockCreated(tb);

    // Clear inputs
    m_nameEdit->clear();
    m_descEdit->clear();
}