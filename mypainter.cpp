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

#if 0
    Q_UNUSED(event);

    QPainter painter(this);
    // 设置画笔颜色
    painter.setPen(QColor(0, 160, 230));

    // 设置字体：微软雅黑、点大小50、斜体
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(50);
    font.setItalic(true);
    painter.setFont(font);

    // 绘制文本
    painter.drawText(rect(), Qt::AlignCenter, "Qt");
#elif 0
    QPainter p(this);
    p.setBrush(QBrush(QColor(121,121,121)));
    p.drawRect(0,0,this->width(),this->height());

    p.setBrush(QBrush(QColor(255,255,255)));
    p.drawRect(0,0,this->width(),this->height());
    std::vector<int> sortcount = count;
    std::sort(sortcount.begin(),sortcount.end());
    int maxcount = sortcount[sortcount.size()-1];


    hist = new QImage(this->width(),this->height(),QImage::Format_RGB888);
    hist->fill(qRgb(255,255,255));
    p.translate(0,hist->height());


    p.drawLine(0,0,100,100);

    int wid=hist->width();
    int hei=hist->height();

    p.drawLine(10,-10,wid-10,-10);// 横轴
    p.drawLine(10,-10,10,-hei+10);//纵轴

    float xstep = float(wid-20) / 256;
    float ystep = float(hei-20) / maxcount;

    for (int i=0;i!=256;i++)
    {
        if(i!=255)
        {
            QColor color(i,255-i,0);
            p.setBrush(color);
            p.setPen(color);
            p.drawRect(10+i*xstep,-10,xstep,-10-ystep*count[i]);
        }

        if(i % 32 == 0||i==255)
        {
            p.drawText(QPointF(10+(i-0.5)*xstep,0),QString::number(i));
        }

    }
#else
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
#endif
}
