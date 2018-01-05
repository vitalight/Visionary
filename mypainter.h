#ifndef MYPAINTER_H
#define MYPAINTER_H
#include <iostream>
#include <QLabel>
#include <QDebug>
#include <QPainter>
#include <algorithm>

#define PAINTER_LINE 1
#define PAINTER_POLYLINE 2
#define PAINTER_EXP 3
#define PAINTER_LOG 4

class MyPainter : public QLabel
{
public:
    MyPainter(QWidget *parent);
    std::vector<double> histogram;
    int lineType = 0;
    double val1 = 0, val2 = 0, val3 = 0, val4 = 0;

    void paintEvent(QPaintEvent *event);

    void painterLine(double gradient, int intercept);
    void painterSection(int pointX1, int pointY1, int pointX2, int pointY2);
    void painterLog(double arga, double argb, double argc);
    void painterExp(double arga, double argb, double argc);
    void clearLine();
};

#endif // MYPAINTER_H
