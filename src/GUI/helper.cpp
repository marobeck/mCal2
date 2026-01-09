#include "helper.h"

#include <QPainter>

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