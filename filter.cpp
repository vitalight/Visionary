#include "filter.h"
#include <QMainWindow>
#include <QRgb>
#include <time.h>

/****************************************************************
 * Global optimization:
 *   Using constBits() instead of getPixel()
 *     # decrease dilation time for large file from 17s to 2.5s
 *
 * Todo:
 *   # User Given Matrix
 *   # More function
 ****************************************************************/
static double responseTime = 0;

double F_responseTime()
{
    return responseTime/1000.0;
}

// test: none
QImage *F_seperation(QImage *image, F_Channel channel)
{
    responseTime = (double)clock();
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index;

    switch (channel) {
        case F_R:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    newBits[index] = qRed(bits[index]);
                }
            }
            break;
        case F_G:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    newBits[index] = qGreen(bits[index]);
                }
            }
            break;
        case F_B:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    newBits[index] = qBlue(bits[index]);
                }
            }
            break;
    }

    responseTime = (double)clock()-responseTime;
    return newImage;
}

// test: none
F_HSI F_RGB2HSI(QRgb rgb)
{
    // is rgb scaled to 1?
    float hue = F_HSI.h, saturation = F_HSI.s, intensity = F_HSI.i;
    float r, g, b;

    float pi = 3.1415926;
    float otz = 2*pi / 3;
    if (hue>=0&&hue<=otz)
    {
        r = intensity*(1+(saturation*cos(hue))/(cos(pi/3.0-hue)));
        b = intensity*(1-saturation);
        g = 3*intensity - (b+r);
    }
    else if (hue>=otz && h<2*otz)
    {
        r = intensity*(1-saturation);
        g = intensity*(1+(saturation*cos(hue-otz))/(cos(pi-hue)));
        b = 3*intensity-(r+g);
    }
    else
    {
        g = intensity*(1-saturation);
        b = intensity*(1+(s*cos(hue-otz*2))/(cos(5*pi/6-h)));
        r = 3*i-(g+b);
    }
}

// test: none
QRgb F_HSI2RGB(F_HSI hsi)
{
    float pi = 3.1415926;
    float tmp = sqrt((r));
    // todo
}

// 彩色->灰度
QImage *F_decolor(QImage *image)
{
    responseTime = (double)clock();
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index;

    for (int i=0; i<newImage->height(); i++) {
        for (int j=0; j<newImage->width(); j++) {
            index = i*newImage->width()+j;
            int avg = (qRed(bits[index]) + qGreen(bits[index]) + qBlue(bits[index]))/3;
            newBits[index] = qRgb(avg, avg, avg);
        }
    }

    responseTime = (double)clock()-responseTime;
    return newImage;
}

QImage *F_binarization(QImage *image, int threshold)
{
    responseTime = (double)clock();
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index, avg, value;

    for (int i=0; i<newImage->height(); i++) {
        for (int j=0; j<newImage->width(); j++) {
            index = i*newImage->width()+j;
            avg = (qRed(bits[index]) + qGreen(bits[index]) + qBlue(bits[index]))/3;
            value = avg>threshold?255:0;
            newBits[index] = qRgb(value, value, value);
        }
    }

    responseTime = (double)clock()-responseTime;
    return newImage;
}

QImage *F_convolution(QImage *image, int kernel[], int kernelSize, int kernelSum)
{
    responseTime = (double)clock();
    int halfSize = kernelSize/2,
        r, g, b, bitsIndex, kernelIndex;
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 0;
            g = 0;
            b = 0;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    bitsIndex = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);
                    kernelIndex = (halfSize+i)*kernelSize+halfSize+j;
                    r += qRed(bits[bitsIndex])*kernel[kernelIndex];
                    g += qGreen(bits[bitsIndex])*kernel[kernelIndex];
                    b += qBlue(bits[bitsIndex])*kernel[kernelIndex];
                }
            }

            r = qBound(0, r/kernelSum, 255);
            g = qBound(0, g/kernelSum, 255);
            b = qBound(0, b/kernelSum, 255);

            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    responseTime = (double)clock()-responseTime;
    return newImage;
}

QImage *F_blur(QImage *image)
{
    int kernel[25] = {1, 4, 7, 4, 1,
                      4, 16, 26, 16, 4,
                      7, 26, 41, 26, 7,
                      4, 16, 26, 16, 4,
                      1, 4, 7, 4, 1};
    return F_convolution(image, kernel, 5, 273);
}

QImage *F_sharpen(QImage *image)
{
    int kernel[9] = {0, -1, 0,
                     -1, 5, -1,
                     0, -1, 0};
    return F_convolution(image, kernel, 3, 1);
}

QImage *F_dilation(QImage *image)
{
    responseTime = (double)clock();
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int kernel[25] = {1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,};
    int kernelSize = 5,
        halfSize = kernelSize/2,
        r, g, b, index;
    QColor color;

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 0;
            g = 0;
            b = 0;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    if (!kernel[(i+halfSize)*kernelSize+j+halfSize] ||
                            x < 0 || x >= image->width() || y < 0 || y >= image->height())
                        continue;
                    index = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);
                    r = r > qRed(bits[index]) ? r : qRed(bits[index]);
                    g = g > qGreen(bits[index]) ? g : qGreen(bits[index]);
                    b = b > qBlue(bits[index]) ? b : qBlue(bits[index]);
                }
            }
            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    responseTime = (double)clock()-responseTime;
    return newImage;
}

QImage *F_erosion(QImage *image)
{
    responseTime = (double)clock();
    QImage *newImage = new QImage(image->size(), QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int kernel[25] = {1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,
                     1, 1, 1, 1, 1,};
    int kernelSize = 5,
        halfSize = kernelSize/2,
        r, g, b, index;
    QColor color;

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 255;
            g = 255;
            b = 255;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    if (!kernel[(i+halfSize)*kernelSize+j+halfSize] ||
                            x < 0 || x >= image->width() || y < 0 || y >= image->height())
                        continue;
                    index = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);
                    r = r < qRed(bits[index]) ? r : qRed(bits[index]);
                    g = g < qGreen(bits[index]) ? g : qGreen(bits[index]);
                    b = b < qBlue(bits[index]) ? b : qBlue(bits[index]);
                }
            }
            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    responseTime = (double)clock()-responseTime;
    return newImage;
}
