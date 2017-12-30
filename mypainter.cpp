#include "mypainter.h"

using namespace std;
#define PADDING 20

MyPainter::MyPainter(QWidget *parent)
    : QLabel(parent)
{
}

void MyPainter::paintEvent(QPaintEvent *event)
{
    qDebug()<<"paintEvent triggered.";
    if (!histogram.size())
        return;

    QPainter p(this);
    int width = this->geometry().width(),
        height = this->geometry().height(),
        x = PADDING,
        y = PADDING,
        recWidth = width - 2 * PADDING,
        recHeight = height - 2 * PADDING;
    double rateMax = *max_element(histogram.begin(), histogram.end()),
           factor = (height - 3 * PADDING) / rateMax,
           step = (width - 2*PADDING)/256.0;

    // draw axis
    p.setBrush(QColor(255, 255, 255));
    p.setPen(QColor(0, 0, 0));
    p.drawRect(x, y, recWidth, recHeight);

    // draw histogram
    for (int i=0; i<256; i++)
    {
        QColor color(0, 0, 0);
        p.setBrush(color);
        p.setPen(color);

        x = PADDING + step*i;
        y = height - PADDING - histogram[i] * factor;
        recWidth = step;
        recHeight = histogram[i] * factor;

        p.drawRect(x, y, recWidth, recHeight);
    }
}
