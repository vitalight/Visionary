#ifndef MYPAINTER_H
#define MYPAINTER_H
#include <iostream>
#include <QLabel>
#include <QDebug>
#include <QPainter>
#include <algorithm>

class MyPainter : public QLabel
{
public:
    MyPainter(QWidget *parent);
    std::vector<double> histogram;

    void paintEvent(QPaintEvent *event);
};

#endif // MYPAINTER_H
