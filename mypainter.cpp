#include "mypainter.h"

using namespace std;
#define PADDING 20
#define PAINTER_X(x) ((x)+PADDING)
#define PAINTER_Y(y) (height-PADDING-(y))
#define PAINTER_POINT(x, y) (QPoint(PAINTER_X(x), PAINTER_Y(y)))

MyPainter::MyPainter(QWidget *parent)
    : QLabel(parent)
{
}

void MyPainter::paintEvent(QPaintEvent *event)
{
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

        x = PAINTER_X(step*i);
        y = PAINTER_Y(histogram[i] * factor);
        recWidth = step;
        recHeight = histogram[i] * factor;

        p.drawRect(x, y, recWidth, recHeight);
    }

    // draw line for contrast operation
    if (lineType) {
        //p.setBrush(QColor(146, 189, 108));
        p.setPen(QColor(146, 189, 108));

        QPoint *points = (QPoint*)malloc(sizeof(QPoint)*256);
        double division = 1;

        switch(lineType)
        {
        case PAINTER_LINE:
        {
            int min_x = 0, max_x = 255;
            if (val2 < 0) {
                min_x = -val2/val1;
            } else if (255*val1+val2 > 255) {
                max_x = (255.0-val2)/val1;
            }
            p.drawLine(PAINTER_POINT(step*min_x, ((min_x*val1 + val2)/255)*(height - 2*PADDING)),
                       PAINTER_POINT(step*max_x, ((max_x*val1 + val2)/255)*(height - 2*PADDING)));
            break;
        }
        case PAINTER_POLYLINE:
            p.drawLine(PAINTER_POINT(0,0),
                       PAINTER_POINT(step*val1, val2/255*(height-2*PADDING)));
            p.drawLine(PAINTER_POINT(step*val1, val2/255*(height-2*PADDING)),
                       PAINTER_POINT(step*val3, val4/255*(height-2*PADDING)));
            p.drawLine(PAINTER_POINT(step*val3, val4/255*(height-2*PADDING)),
                       PAINTER_POINT(step*255, height-2*PADDING));
            break;
        case PAINTER_EXP:
            division = pow(255, val1+1);
            for (int i = 0; i < 256; i++) {
                points[i] = PAINTER_POINT(step*i, pow(i, val1+1)/division*(height - 2*PADDING));
            }
            p.drawPolyline(points, 256);
            break;
        case PAINTER_LOG:
            division = log(256*(val1+0.1));
            for (int i = 0; i < 256; i++) {
                points[i] = PAINTER_POINT(step*i, log((1+i)*(val1+0.1))/division*(height - 2*PADDING));
            }
            p.drawPolyline(points, 256);
            break;
        }
        //p.drawLine(PAINTER_POINT(0, 0), PAINTER_POINT(step*255, height - 2*PADDING));
    }
}

void MyPainter::painterLine(double gradient, int intercept)
{
    lineType = PAINTER_LINE;
    val1 = gradient;
    val2 = intercept;
}

void MyPainter::painterSection(int pointX1, int pointY1, int pointX2, int pointY2)
{
    lineType = PAINTER_POLYLINE;
    val1 = pointX1;
    val2 = pointY1;
    val3 = pointX2;
    val4 = pointY2;
}

void MyPainter::painterLog(double factor)
{
    lineType = PAINTER_LOG;
    val1 = factor;
}

void MyPainter::painterExp(double power)
{
    lineType = PAINTER_EXP;
    val1 = power;
}

void MyPainter::clearLine()
{
    lineType = 0;
}
