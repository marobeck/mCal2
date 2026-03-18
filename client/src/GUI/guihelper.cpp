#include "guihelper.h"

#include <QPainter>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QDialogButtonBox>

QIcon makeColorIcon(const QColor &c, const QSize &sz)
{
    QPixmap pm(sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    QRectF r(0, 0, sz.width(), sz.height());
    p.drawEllipse(r);
    p.end();
    return QIcon(pm);
};

QDate getDateFromUser(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QDate &defaultDate)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *textLabel = new QLabel(label, &dialog);
    layout->addWidget(textLabel);

    QDateEdit *dateEdit = new QDateEdit(defaultDate, &dialog);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout->addWidget(dateEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dialog
    );
    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted,
                     &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected,
                     &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
        return dateEdit->date();

    return QDate();  // invalid date == canceled
}
