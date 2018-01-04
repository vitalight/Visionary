#include "filter.h"

using namespace std;

/****************************************************************
 * Global optimization:
 *   Using constBits() instead of getPixel()
 *     # decrease dilation time for large file from 17s to 2.5s
 *
 * Todo:
 *   # User Given Matrix
 *   # More function
 ****************************************************************/
#define F_NEW_IMAGE(image) (new QImage((image)->size(), QImage::Format_ARGB32))
#define TIMMING_BEGIN (responseTime = (double)clock())
#define TIMMING_END (responseTime = (double)clock()-responseTime)
static double responseTime = 0;
static const double PI = 4.0*atan(1.0);

double F_responseTime()
{
    return responseTime/1000.0;
}

// this is a example function
QImage *F_example(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            newBits[y*width+x] = bits[y*width+x];
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_tag(QImage *image, int x, int y)
{
    QImage *newImage = new QImage(*image);
    int width = image->width(), height = image->height();
    QRgb *newBits = (QRgb*)newImage->bits();
    newBits[y*width+x] = qRgb(0, 0, 255);
    if (U_legal(width, height, x, y-1)) {
        newBits[y*width+x-width] = qRgb(0, 0, 255);
    }
    if (U_legal(width, height, x, y+1)) {
        newBits[y*width+x+width] = qRgb(0, 0, 255);
    }
    if (U_legal(width, height, x-1, y)) {
        newBits[y*width+x-1] = qRgb(0, 0, 255);
    }
    if (U_legal(width, height, x+1, y)) {
        newBits[y*width+x+1] = qRgb(0, 0, 255);
    }
    return newImage;
}
/***************************************************************
 * 1. 彩色图像处理
****************************************************************/
// 三通道分离
// [test] some
QImage *F_seperation(QImage *image, F_Channel channel)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index, color;

    switch (channel) {
        case F_R:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    color = qRed(bits[index]);
                    newBits[index] = qRgb(color, 0, 0);
                }
            }
            break;
        case F_G:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    color = qGreen(bits[index]);
                    newBits[index] = qRgb(0, color, 0);
                }
            }
            break;
        case F_B:
            for (int i=0; i<newImage->height(); i++) {
                for (int j=0; j<newImage->width(); j++) {
                    index = i*newImage->width()+j;
                    color = qBlue(bits[index]);
                    newBits[index] = qRgb(0, 0, color);
                }
            }
            break;
    }

    TIMMING_END;
    return newImage;
}

// 彩色->灰度
QImage *F_decolor(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index;

    for (int i=0; i<newImage->height(); i++) {
        for (int j=0; j<newImage->width(); j++) {
            index = i*newImage->width()+j;
            // Gray = R*0.299 + G*0.587 + B*0.114
            int avg = qRed(bits[index])*0.299 + qGreen(bits[index])*0.587 + qBlue(bits[index])*0.114;
            newBits[index] = qRgb(avg, avg, avg);
        }
    }

    TIMMING_END;
    return newImage;
}

// 色相/饱和度/亮度调节
F_HSB F_RGB2HSB(QRgb rgb)
{
    int rgbR = qRed(rgb),
        rgbG = qGreen(rgb),
        rgbB = qBlue(rgb),
        maxVal = max(max(rgbR, rgbG), rgbB),
        minVal = min(min(rgbR, rgbG), rgbB);

    double hsbB = maxVal/255.0f,
           hsbS = maxVal==0 ? 0 : (maxVal-minVal) / (double)maxVal,
           hsbH = 0;

    if (maxVal == rgbR && rgbG >= rgbB) {
        hsbH = (rgbG - rgbB) * 60.0f / (maxVal - minVal);
    } else if (maxVal == rgbR && rgbG < rgbB) {
        hsbH = (rgbG - rgbB) * 60.0f / (maxVal - minVal) + 360;
    } else if (maxVal == rgbG) {
        hsbH = (rgbB - rgbR) * 60.0f / (maxVal - minVal) + 120;
    } else if (maxVal == rgbB) {
        hsbH = (rgbR - rgbG) * 60.0f / (maxVal - minVal) + 240;
    }

    return F_HSB(hsbH, hsbS, hsbB);
}

// [input] 0~360, 0~1, 0~1
QRgb F_HSB2RGB(F_HSB hsb)
{
    double r = 0, g = 0, b = 0;
    int i = (int)(hsb.h/60) % 6;
    double f = (hsb.h/60)-i,
           p = hsb.b * (1 - hsb.s),
           q = hsb.b * (1 - f * hsb.s),
           t = hsb.b * (1 - (1 - f) * hsb.s);

    switch(i) {
    case 0:
        r = hsb.b;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = hsb.b;
        b = p;
        break;
    case 2:
        r = p;
        g = hsb.b;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = hsb.b;
        break;
    case 4:
        r = t;
        g = p;
        b = hsb.b;
        break;
    case 5:
        r = hsb.b;
        g = p;
        b = q;
        break;
    }
    return qRgb(r*255, g*255, b*255);
}

QImage *F_adjustHSB(QImage *image, int h_val, int s_val, int b_val)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            F_HSB hsb = F_RGB2HSB(bits[y*width+x]);

            // do something with fsb;
            hsb.h = (int)(hsb.h+h_val+360) % 360;
            hsb.s *= 1.0 + s_val/100.0;
            if (hsb.s>1)
                hsb.s = 1;
            hsb.b *= 1.0 + b_val/100.0;
            if (hsb.b>1)
                hsb.b = 1;

            newBits[y*width+x] = F_HSB2RGB(hsb);
        }
    }

    TIMMING_END;
    return newImage;
}

// 色阶调整
QImage *F_colorGradation(QImage *image, int shadow, double midtone, int highlight)
{
    TIMMING_BEGIN;

    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), r, g, b;

    double diff = highlight - shadow;
    double power = std::min(1.0/midtone, 100.0);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = qBound(0, int(255 * pow((qRed(bits[y*width+x]) - shadow)/diff, power)), 255);
            g = qBound(0, int(255 * pow((qGreen(bits[y*width+x]) - shadow)/diff, power)), 255);
            b = qBound(0, int(255 * pow((qBlue(bits[y*width+x]) - shadow)/diff, power)), 255);
            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}
/***************************************************************
 * 2. 二值化
****************************************************************/
QImage *F_binarization(QImage *image, int threshold)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
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

    TIMMING_END;
    return newImage;
}
// Otus(大津算法)
QImage *F_binarization_Otsu(QImage *image)
{
    int threshold=0;
    vector<double> histogram = F_getHistogram(image);

    /********************************************
     *  g = w0/(1-w0) * (u0-u)^2
     * ******************************************
     *  g: variance
     *  w0: foreground rate
     *  u: total grey average
     *  u0: foreground grey average
     *  choose the threshold that has greatest g;
     ********************************************/
    double variance, varianceMax = 0,
           foregroundRate = 0,
           foregroundSum = 0,
           avg = 0,
           u0_minus_u;

    for (int i = 0; i < 256; i++)
    {
        avg += i * histogram[i];
    }

    for (int i = 0; i < 256; i++)
    {
        foregroundRate += histogram[i];
        foregroundSum += i * histogram[i];
        u0_minus_u = foregroundSum/foregroundRate - avg,
        variance = foregroundRate/(1-foregroundRate) * pow(u0_minus_u, 2);
        if (variance > varianceMax)
        {
            varianceMax = variance;
            threshold = i;
        }
    }

    return F_binarization(image, threshold);
}
// 手动调节：双阈值，实时
QImage *F_binarization_double(QImage *image, int threshold_low, int threshold_high)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int index, avg, value;

    for (int i=0; i<newImage->height(); i++) {
        for (int j=0; j<newImage->width(); j++) {
            index = i*newImage->width()+j;
            avg = (qRed(bits[index]) + qGreen(bits[index]) + qBlue(bits[index]))/3;
            value = avg > threshold_low && avg < threshold_high ? 255 : 0;
            newBits[index] = qRgb(value, value, value);
        }
    }

    TIMMING_END;
    return newImage;
}

/***************************************************************
 * 3. 代数与几何操作
****************************************************************/
// 加
QImage *F_add(QImage *image1, QImage *image2)
{
    TIMMING_BEGIN;
    int width1 = image1->width(), height1 = image1->height(),
        width2 = image2->width(), height2 = image2->height(),
        width = max(width1, width2),
        height = max(height1, height2),
        r, g, b;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits1 = (QRgb*)image1->constBits(),
         *bits2 = (QRgb*)image2->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = (U_legal(width1, height1, x, y) ? qRed(bits1[y*width1+x]) : 0) +
                   (U_legal(width2, height2, x, y) ? qRed(bits2[y*width2+x]) : 0);
            g = (U_legal(width1, height1, x, y) ? qGreen(bits1[y*width1+x]) : 0) +
                   (U_legal(width2, height2, x, y) ? qGreen(bits2[y*width2+x]) : 0);
            b = (U_legal(width1, height1, x, y) ? qBlue(bits1[y*width1+x]) : 0) +
                   (U_legal(width2, height2, x, y) ? qBlue(bits2[y*width2+x]) : 0);

            U_colorBound(r, g, b);

            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 减
QImage *F_minus(QImage *image1, QImage *image2)
{
    TIMMING_BEGIN;
    int width1 = image1->width(), height1 = image1->height(),
        width2 = image2->width(), height2 = image2->height(),
        width = max(width1, width2),
        height = max(height1, height2),
        r, g, b;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits1 = (QRgb*)image1->constBits(),
         *bits2 = (QRgb*)image2->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = (U_legal(width1, height1, x, y) ? qRed(bits1[y*width1+x]) : 0) -
                   (U_legal(width2, height2, x, y) ? qRed(bits2[y*width2+x]) : 0);
            g = (U_legal(width1, height1, x, y) ? qGreen(bits1[y*width1+x]) : 0) -
                   (U_legal(width2, height2, x, y) ? qGreen(bits2[y*width2+x]) : 0);
            b = (U_legal(width1, height1, x, y) ? qBlue(bits1[y*width1+x]) : 0) -
                   (U_legal(width2, height2, x, y) ? qBlue(bits2[y*width2+x]) : 0);

            U_colorBound(r, g, b);

            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 乘
QImage *F_times(QImage *image1, QImage *image2)
{
    TIMMING_BEGIN;
    int width1 = image1->width(), height1 = image1->height(),
        width2 = image2->width(), height2 = image2->height(),
        width = max(width1, width2),
        height = max(height1, height2),
        r, g, b;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits1 = (QRgb*)image1->constBits(),
         *bits2 = (QRgb*)image2->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = (U_legal(width1, height1, x, y) ? qRed(bits1[y*width1+x]) : 0) *
                   (U_legal(width2, height2, x, y) ? qRed(bits2[y*width2+x]) : 0);
            g = (U_legal(width1, height1, x, y) ? qGreen(bits1[y*width1+x]) : 0) *
                   (U_legal(width2, height2, x, y) ? qGreen(bits2[y*width2+x]) : 0);
            b = (U_legal(width1, height1, x, y) ? qBlue(bits1[y*width1+x]) : 0) *
                   (U_legal(width2, height2, x, y) ? qBlue(bits2[y*width2+x]) : 0);

            U_colorBound(r, g, b);

            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 裁剪
QImage *F_cut(QImage *image, int x_start, int y_start, int width, int height)
{
    TIMMING_BEGIN;
    int oldWidth = image->width(), oldHeight = image->height(),
        newWidth = min(oldWidth-x_start, width), newHeight = min(oldHeight-y_start, height);
    QImage *newImage = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < newHeight; y++)
    {
        for (int x = 0; x < newWidth; x++)
        {
            newBits[y * newWidth + x] = bits[(y + y_start) * oldWidth + x + x_start];
        }
    }
    TIMMING_END;
    return newImage;
}

// 双线性插值缩放
QImage *F_resize_linear(QImage *image, int width, int height)
{
    TIMMING_BEGIN;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    int oldWidth = image->width(),
        oldHeight = image->height(),
        target_w, target_h,
        r, g, b, index;

    double rate_w = double(oldWidth - 1) / (width - 1),
          rate_h = double(oldHeight - 1) / (height - 1),
          decimal_w, decimal_h,
          left_y, right_y, left_x, right_x;

    // todo: deal with 0 seperately
    for (int y = 0; y < height; y++)
    {
        decimal_h = y * rate_h;
        target_h = int(decimal_h);
        left_y = decimal_h - target_h;
        right_y = 1 - left_y;
        for (int x = 0; x < width; x++)
        {
            decimal_w = x * rate_w;
            target_w = int(decimal_w);
            left_x = decimal_w - target_w;
            right_x = 1 - left_x;

            index = target_w + target_h * oldWidth;

            if (x == width - 1) {
                if (y == height - 1) {
                    r = qRed(bits[index]);
                    g = qGreen(bits[index]);
                    b = qBlue(bits[index]);
                } else {
                    r = right_y * qRed(bits[index])  + left_y * qRed(bits[index + oldWidth]);
                    g = right_y * qGreen(bits[index]) + left_y * qGreen(bits[index + oldWidth]);
                    b = right_y * qBlue(bits[index]) + left_y * qBlue(bits[index + oldWidth]);
                }
            } else {
                if (y == height - 1) {
                    r = right_x * qRed(bits[index]) + left_x * qRed(bits[index + 1]);
                    g = right_x * qGreen(bits[index]) + left_x * qGreen(bits[index + 1]);
                    b = right_x * qBlue(bits[index]) + left_x * qBlue(bits[index + 1]);
                } else {
                    r = right_y * (right_x * qRed(bits[index]) + left_x * qRed(bits[index + 1]))
                            + left_y * (right_x * qRed(bits[index + oldWidth]) + left_x * qRed(bits[index + oldWidth + 1]));
                    g = right_y * (right_x * qGreen(bits[index]) + left_x * qGreen(bits[index + 1]))
                            + left_y * (right_x * qGreen(bits[index + oldWidth]) + left_x * qGreen(bits[index + oldWidth + 1]));
                    b = right_y * (right_x * qBlue(bits[index]) + left_x * qBlue(bits[index + 1]))
                            + left_y * (right_x * qBlue(bits[index + oldWidth]) + left_x * qBlue(bits[index + oldWidth + 1]));
                }
            }
            newBits[x + y * width] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 最近邻缩放
QImage *F_resize_nearest(QImage *image, int width, int height)
{
    TIMMING_BEGIN;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    int oldWidth = image->width(),
        oldHeight = image->height(),
        target_w, target_h;

    double rate_w = double(oldWidth - 1) / (width - 1),
          rate_h = double(oldHeight - 1) / (height - 1);

    for (int y = 0; y < height; y++)
    {
        target_h = U_round(y * rate_h);
        for (int x = 0; x < width; x++)
        {
            target_w = U_round(x* rate_w);
            newBits[x + y * width] = bits[target_w + target_h * oldWidth];
        }
    }
    TIMMING_END;
    return newImage;
}

QImage *F_resize(QImage *image, int width, int height, F_ScaleAlgo algo)
{
    if (algo == F_NEAREST)
        return F_resize_nearest(image, width, height);
    else
        return F_resize_linear(image, width, height);
}

// 裁剪透明区域
QImage *F_cut_transparent(QImage *image)
{
    QRgb *bits = (QRgb *)image->constBits();
    int width = image->width(), height =image->height(),
        min_x = width, min_y = height, max_x = 0, max_y = 0, index = 0;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            index++;
            if (qAlpha(bits[index])) {
                if (x < min_x)
                    min_x = x;
                else if (x > max_x)
                    max_x = x;
                if (y < min_y)
                    min_y = y;
                else if (y > max_y)
                    max_y = y;
            }
        }
    }

    // if no cut is needed, return original image
    if (max_x == width - 1 && min_x == 0 && max_y == height - 1 && min_y == 0)
    {
        return image;
    }
    int newWidth = max_x - min_x + 1, newHeight = max_y - min_y + 1;
    QImage *newImage = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
    QRgb *newBits = (QRgb *)newImage->bits();
    index = 0;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            newBits[index++] = bits[y * width + x];
        }
    }

    return newImage;
}

// 双线性插值旋转
QImage *F_spin_linear(QImage *image, int angle)
{
    TIMMING_BEGIN;
    double theta = angle/180.0*PI;
    int width = image->width(), height = image->height(),
        newWidth = ceil(abs(width*cos(theta)) + abs(height*sin(theta))),
        newHeight = ceil(abs(height*cos(theta)) + abs(width*sin(theta))),
        index, target_index,
        target_x, target_y,
        left, right, top, bottom,
        r, g, b;
    double decimal_x, decimal_y,
          delta_x = - 0.5 * newWidth*cos(theta) - 0.5 * newHeight*sin(theta) + 0.5 * width,
          delta_y = 0.5 * newWidth*sin(theta) - 0.5 * newHeight*cos(theta) + 0.5 * height;

    QImage *newImage = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int y = 0; y < newHeight; y++)
    {
        for (int x = 0; x < newWidth; x++)
        {
            decimal_x = x*cos(theta) + y*sin(theta) + delta_x;
            decimal_y = -x*sin(theta) + y*cos(theta) + delta_y;

            target_x = (int)decimal_x;
            target_y = (int)decimal_y;

            left = decimal_x - target_x;
            right = 1-left;
            top = decimal_y - target_y;
            bottom = 1-top;

            index = y * newWidth + x;
            target_index = target_x + target_y * width;

            if ((target_x && target_y &&
                !U_legal(width, height, decimal_x-0.1, decimal_y-0.1)) ||
                !qAlpha(bits[target_index])) {
                newBits[index] = qRgb(255, 255, 255);
                continue;
            }


            if (target_x == width) {
                if (target_y == height) {
                    r = qRed(bits[target_index]);
                    g = qGreen(bits[target_index]);
                    b = qBlue(bits[target_index]);
                } else {
                    r = bottom * qRed(bits[target_index]) + top * qRed(bits[target_index+width]);
                    g = bottom * qGreen(bits[target_index]) + top * qGreen(bits[target_index+width]);
                    b = bottom * qBlue(bits[target_index]) + top * qBlue(bits[target_index+width]);
                }
            } else {
                if (target_y == height) {
                    r = right * qRed(bits[target_index]) + left * qRed(bits[target_index+1]);
                    g = right * qGreen(bits[target_index]) + left * qGreen(bits[target_index+1]);
                    b = right * qBlue(bits[target_index]) + left * qBlue(bits[target_index+1]);
                } else {
                    r = right * (bottom * qRed(bits[target_index]) + top * qRed(bits[target_index+width]))
                        + left * (bottom * qRed(bits[target_index+1]) + top * qRed(bits[target_index+width+1]));
                    g = right * (bottom * qGreen(bits[target_index]) + top * qGreen(bits[target_index+width]))
                        + left * (bottom * qGreen(bits[target_index+1]) + top * qGreen(bits[target_index+width+1]));
                    b = right * (bottom * qBlue(bits[target_index]) + top * qBlue(bits[target_index+width]))
                        + left * (bottom * qBlue(bits[target_index+1]) + top * qBlue(bits[target_index+width+1]));
                }
            }
            newBits[index] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 最近邻旋转
// [improve] border missing dots
QImage *F_spin_nearest(QImage *image, int angle)
{
    TIMMING_BEGIN;
    double theta = (angle%360)/180.0*PI;
    int width = image->width(), height = image->height(),
        newWidth = ceil(abs(width*cos(theta)) + abs(height*sin(theta))),
        newHeight = ceil(abs(height*cos(theta)) + abs(width*sin(theta))),
        index, target_index,
        target_x, target_y;
    double decimal_x, decimal_y,
          delta_x = - 0.5 * newWidth*cos(theta) - 0.5 * newHeight*sin(theta) + 0.5 * width,
          delta_y = 0.5 * newWidth*sin(theta) - 0.5 * newHeight*cos(theta) + 0.5 * height;

    QImage *newImage = new QImage(newWidth, newHeight, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int y = 0; y < newHeight; y++)
    {
        for (int x = 0; x < newWidth; x++)
        {
            decimal_x = x*cos(theta) + y*sin(theta) + delta_x;
            decimal_y = -x*sin(theta) + y*cos(theta) + delta_y;

            target_x = U_round(decimal_x);
            target_y = U_round(decimal_y);

            index = y * newWidth + x;
            // ignore blank space
            if (!U_legal(width, height, target_x, target_y)) {
                newBits[index] = qRgb(255, 255, 255);
                continue;
            }
            target_index = target_x + target_y * width;
            newBits[index] = bits[target_index];
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_spin(QImage *image, int angle, F_ScaleAlgo algo)
{
    if (algo == F_NEAREST)
        return F_spin_nearest(image, angle);
    else
        return F_spin_linear(image, angle);
}

/***************************************************************
 * 4. 对比度调节
****************************************************************/
// 线性及分段线性调整
QImage *F_contrast_linear(QImage *image, double gradient, int intercept)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), r, g, b;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = qBound(0, int(qRed(bits[y*width+x]) * gradient + intercept), 255);
            g = qBound(0, int(qGreen(bits[y*width+x]) * gradient + intercept), 255);
            b = qBound(0, int(qBlue(bits[y*width+x]) * gradient + intercept), 255);
            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_contrast_section(QImage *image, int pointX1, int pointY1, int pointX2, int pointY2)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(),
        r, g, b;
    double gradient, intercept;

    pointX1 = min(pointX1, pointX2);
    pointX2 = max(pointX1, pointX2);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (qRed(bits[y*width+x])<pointX1) {
                gradient = double(pointY1)/pointX1;
                intercept = 0;
            } else if (qRed(bits[y*width+x])<pointX2) {
                gradient = double(pointY2-pointY1)/(pointX2-pointX1);
                intercept = pointY1 - pointX1 * gradient;
            } else if (qRed(bits[y*width+x])<256){
                gradient = double(255-pointY2)/(255-pointX1);
                intercept = pointY2 - pointX2 * gradient;
            }

            r = qBound(0, int(qRed(bits[y*width+x]) * gradient + intercept), 255);

            if (qGreen(bits[y*width+x])<pointX1) {
                gradient = double(pointY1)/pointX1;
                intercept = 0;
            } else if (qGreen(bits[y*width+x])<pointX2) {
                gradient = double(pointY2-pointY1)/(pointX2-pointX1);
                intercept = pointY1 - pointX1 * gradient;
            } else if (qGreen(bits[y*width+x])<256){
                gradient = double(255-pointY2)/(255-pointX1);
                intercept = pointY2 - pointX2 * gradient;
            }
            g = qBound(0, int(qGreen(bits[y*width+x]) * gradient + intercept), 255);

            if (qBlue(bits[y*width+x])<pointX1) {
                gradient = double(pointY1)/pointX1;
                intercept = 0;
            } else if (qBlue(bits[y*width+x])<pointX2) {
                gradient = double(pointY2-pointY1)/(pointX2-pointX1);
                intercept = pointY1 - pointX1 * gradient;
            } else if (qBlue(bits[y*width+x])<256){
                gradient = double(255-pointY2)/(255-pointX1);
                intercept = pointY2 - pointX2 * gradient;
            }
            b = qBound(0, int(qBlue(bits[y*width+x]) * gradient + intercept), 255);
            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 非线性调整：对数、指数（系数可调）
QImage *F_contrast_logarithm(QImage *image, double factor)
{
    TIMMING_BEGIN;
    factor += 0.1;

    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), r, g, b;
    double division = log(256*factor)/256;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = qBound(0, int(log(1+qRed(bits[y*width+x])*factor)/division), 255);
            g = qBound(0, int(log(1+qGreen(bits[y*width+x])*factor)/division), 255);
            b = qBound(0, int(log(1+qBlue(bits[y*width+x])*factor)/division), 255);
            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_contrast_exponential(QImage *image, double power)
{
    TIMMING_BEGIN;
    power += 1;

    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), r, g, b;
    long long int division = pow(255, power-1);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = qBound(0, int(pow(qRed(bits[y*width+x]), power) / division), 255);
            g = qBound(0, int(pow(qGreen(bits[y*width+x]), power) / division), 255);
            b = qBound(0, int(pow(qBlue(bits[y*width+x]), power) / division), 255);
            newBits[y*width+x] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 图像的直方图显示
vector<double> F_getHistogram(QImage *image)
{
    vector<double> histogram(256);

    if (!image) {
        return histogram;
    }

    QRgb *bits = (QRgb *)image->constBits();
    int index,
        width = image->width(),
        height = image->height(),
        numberOfPixal = width*height;

    for (int x=0; x<width; x++)
    {
        for (int y=0; y<height; y++)
        {
            index = y*image->width()+x;
            histogram[qRed(bits[index])]++;
        }
    }

    for (int i = 0; i < 256; i++)
    {
            histogram[i] /= numberOfPixal;
    }

    return histogram;
}

void F_getHistogram_rgb(QImage *image, vector<int> &r_h, vector<int> &g_h, vector<int> &b_h)
{
    QRgb *bits = (QRgb *)image->constBits();
    int index, value,
        width = image->width(),
        height = image->height();

    for (int x=0; x<width; x++)
    {
        for (int y=0; y<height; y++)
        {
            index = y*image->width()+x;
            value = qRed(bits[index]);
            r_h[value]++;
            value = qGreen(bits[index]);
            g_h[value]++;
            value = qBlue(bits[index]);
            b_h[value]++;
        }
    }
}

// 直方图均衡化
QImage *F_equalizeHistogram(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    // rgb histogram
    vector<int> r_h(256), g_h(256), b_h(256);
    // rgb proportion
    vector<double> r_pro(256), g_pro(256), b_pro(256);
    // rgb table
    vector<int> r_table(256), g_table(256), b_table(256);
    int index,
        width = image->width(),
        height = image->height(),
        numberOfPixal = width * height;

    F_getHistogram_rgb(image, r_h, g_h, b_h);

    r_pro[0] = r_h[0] * 255.0 / numberOfPixal;
    g_pro[0] = g_h[0] * 255.0 / numberOfPixal;
    b_pro[0] = b_h[0] * 255.0 / numberOfPixal;
    for (int i = 1; i < 256; i++)
    {
        r_pro[i] = r_pro[i-1] + r_h[i] * 255.0 / numberOfPixal;
        r_table[i] = int(r_pro[i]+0.5);
        g_pro[i] = g_pro[i-1] + g_h[i] * 255.0 / numberOfPixal;
        g_table[i] = int(g_pro[i]+0.5);
        b_pro[i] = b_pro[i-1] + b_h[i] * 255.0 / numberOfPixal;
        b_table[i] = int(b_pro[i]+0.5);
    }

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            index = x + y * width;
            newBits[index] = qRgb(r_table[qRed(bits[index])],
                                  g_table[qGreen(bits[index])],
                                  b_table[qBlue(bits[index])]);
        }
    }
    TIMMING_END;
    return newImage;
}

/***************************************************************
 * 5. 平滑滤波器（卷积核允许用户自定义）
****************************************************************/
// [reminder] kernelSum is necessary because some call need no normalization
QImage *F_convolution(QImage *image, U_Kernel_d kernel, double kernelSum)
{
    TIMMING_BEGIN;
    int halfSize = kernel.size()/2,
        bitsIndex,
        r, g, b;
    double d_r, d_g, d_b;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            d_r = 0;
            d_g = 0;
            d_b = 0;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    bitsIndex = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);
                    d_r += qRed(bits[bitsIndex]) * kernel[halfSize+j][halfSize+i];
                    d_g += qGreen(bits[bitsIndex]) * kernel[halfSize+j][halfSize+i];
                    d_b += qBlue(bits[bitsIndex]) * kernel[halfSize+j][halfSize+i];
                }
            }

            r = qBound(0, U_round(d_r/kernelSum), 255);
            g = qBound(0, U_round(d_g/kernelSum), 255);
            b = qBound(0, U_round(d_b/kernelSum), 255);

            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    TIMMING_END;
    return newImage;
}

// 均值
QImage *F_blur_mean(QImage *image, int radius)
{
    U_Kernel_d kernel = U_getFlatKernel_d(radius);
    return F_convolution(image, kernel, radius*radius);
}

// 中值
QImage *F_blur_median(QImage *image, int radius)
{
    TIMMING_BEGIN;
    int halfSize = radius/2,
        bitsIndex,
        r, g, b;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {

            vector<int> median_r, median_g, median_b;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    bitsIndex = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);

                    median_r.push_back(qRed(bits[bitsIndex]));
                    median_g.push_back(qGreen(bits[bitsIndex]));
                    median_b.push_back(qBlue(bits[bitsIndex]));
                }
            }

            sort(median_r.begin(), median_r.end());
            sort(median_g.begin(), median_g.end());
            sort(median_b.begin(), median_b.end());

            r = median_r[radius/2];
            g = median_g[radius/2];
            b = median_b[radius/2];

            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    TIMMING_END;
    return newImage;
}

// 高斯
QImage *F_blur_gaussian(QImage *image, int radius, double sigma)
{
    U_Kernel_d kernel = U_getGaussianKernel(radius, sigma);
    return F_convolution(image, kernel, 1);
}

// 锐化（自定）
QImage *F_sharpen(QImage *image)
{
    U_Kernel_d kernel = {{0, -1,  0},
                         {-1, 5, -1},
                         {0, -1,  0}};
    return F_convolution(image, kernel, 1);
}

/***************************************************************
 * 6. 边缘检测
****************************************************************/
// Sobel
QImage *F_detectEdge_sobel(QImage *image)
{
    TIMMING_BEGIN;
    U_Kernel_i kernel_x = {{-1, 0, 1},
                                    {-2, 0, 2},
                                    {-1, 0, 1}};
    U_Kernel_i kernel_y = {{-1, -2, -1},
                                    { 0,  0,  0},
                                    { 1,  2,  1}};

    int halfSize = 1,
        r_x, g_x, b_x,
        r_y, g_y, b_y,
        r, g, b, bitsIndex;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r_x = 0;
            g_x = 0;
            b_x = 0;
            r_y = 0;
            g_y = 0;
            b_y = 0;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    bitsIndex = qBound(0, x+i, image->width()-1)
                            + image->width() * qBound(0, y+j, image->height()-1);

                    r_x += qRed(bits[bitsIndex]) * kernel_x[halfSize+j][halfSize+i];
                    g_x += qGreen(bits[bitsIndex]) * kernel_x[halfSize+j][halfSize+i];
                    b_x += qBlue(bits[bitsIndex]) * kernel_x[halfSize+j][halfSize+i];

                    r_y += qRed(bits[bitsIndex]) * kernel_y[halfSize+j][halfSize+i];
                    g_y += qGreen(bits[bitsIndex]) * kernel_y[halfSize+j][halfSize+i];
                    b_y += qBlue(bits[bitsIndex]) * kernel_y[halfSize+j][halfSize+i];
                }
            }

            r = qBound(0, U_round(sqrt(pow(r_x, 2) + pow(r_y, 2))), 255);
            g = qBound(0, U_round(sqrt(pow(g_x, 2) + pow(g_y, 2))), 255);
            b = qBound(0, U_round(sqrt(pow(b_x, 2) + pow(b_y, 2))), 255);
            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    TIMMING_END;
    return newImage;
}
// 拉普拉斯
QImage *F_detectEdge_laplacian(QImage *image)
{
    U_Kernel_d kernel = {{0, 1, 0},
                                     {1,-4, 1},
                                     {0, 1, 0}};
    return F_convolution(image, kernel, 1);
}
// canny
QImage *F_detectEdge_canny(QImage *image)
{
    TIMMING_BEGIN;
    int threshold_low = 55, threshold_high = 90;

    U_Kernel_i kernel_x = {{-1,  0,  1},
                                    {-2,  0,  2},
                                    {-1,  0,  1}};
    U_Kernel_i kernel_y = {{-1, -2, -1},
                                    { 0,  0,  0},
                                    { 1,  2,  1}};
    U_Kernel_i direction;

    int halfSize = 1,
        sum_x, sum_y,
        color, bitsIndex,
        width = image->width(),
        height = image->height();
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    double theta;

    /********************************************
     * sobel algorithm
     ********************************************/
    for (int x = 0; x < width; x++) {
        vector<int> line;

        for (int y = 0; y < height; y++) {
            sum_x = 0;
            sum_y = 0;

            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    bitsIndex = qBound(0, x+i, width-1)
                            + width * qBound(0, y+j, height-1);

                    sum_x += qRed(bits[bitsIndex]) * kernel_x[halfSize+j][halfSize+i];
                    sum_y += qRed(bits[bitsIndex]) * kernel_y[halfSize+j][halfSize+i];
                }
            }

            theta = sum_x!=0? atan(sum_y/sum_x) : PI/4;

            theta = theta / PI * 8;

            line.push_back(((int)(theta+8))%8);

            color = qBound(0, U_round(sqrt(pow(sum_x, 2) + pow(sum_y, 2))), 255);
            newBits[y*width+x] = qRgb(color, color, color);
        }
        direction.push_back(line);
    }

    /********************************************
     * 非最大值抑制
     ********************************************/
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            switch(direction[y][x])
            {
            // right
            case 0:
            case 7:
                if (!((x==0 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x-1])) &&
                    (x==width-1 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x+1])))) {
                    newBits[y*width+x] = qRgb(0, 0, 0);
                }
                break;
            // northeast
            case 1:
            case 2:
                if (!((x==width-1 || y==0  || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x-width+1])) &&
                    (x==0 || y==height-1 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x+width-1])))) {
                    newBits[y*width+x] = qRgb(0, 0, 0);
                }
                break;
            // north
            case 3:
            case 4:
                if (!((y==0 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x-width])) &&
                    (y==height-1 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x+width])))) {
                    newBits[y*width+x] = qRgb(0, 0, 0);
                }
                break;
            // southeast
            case 5:
            case 6:
                if (!((x==0 || y==0 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x-width-1])) &&
                    (x==width-1 || y==height-1 || qRed(newBits[y*width+x])>=qRed(newBits[y*width+x+width+1])))) {
                    newBits[y*width+x] = qRgb(0, 0, 0);
                }
                break;
            default:
                Q_ASSERT(0);
            }
        }
    }

    /********************************************
     * 双阈值及边界跟踪
     ********************************************/
    // 初始化visited矩阵用于边界跟踪
    vector<vector<bool>> visited;
    for (int y = 0; y < height; y++)
    {
        vector<bool> line(width, false);
        visited.push_back(line);
    }

    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            if (visited[y][x])
                continue;

            color = qRed(newBits[y*width+x]);
            if (color < threshold_low){
                visited[y][x] = true;
                newBits[y*width+x] = qRgb(0, 0, 0);
            } else if (color >= threshold_high) {
                visited[y][x] = true;

                // track this edge
                stack<U_Point> edge;
                U_Point point = U_Point(x, y);
                edge.push(point);

                while(edge.size())
                {
                    point = edge.top();
                    newBits[point.x+point.y*width] = qRgb(255, 255, 255);
                    edge.pop();

                    for (int i = -1; i < 2; i++)
                    {
                        for (int j = -1; j < 2; j++)
                        {
                            int newy = y+j, newx = x+i;
                            if(U_legal(width, height, newx, newy) &&
                               !visited[newy][newx] &&
                               qRed(newBits[newy*width+newx]) >= threshold_low) {
                                edge.push(U_Point(newx, newy));
                                visited[newy][newx] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            if (!visited[y][x])
                newBits[y*width+x] = qRgb(0, 0, 0);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_detectEdge(QImage *image, F_DetectEdgeAlgo algo)
{
    switch (algo)
    {
    case F_SOBEL:
        return F_detectEdge_sobel(F_decolor(F_blur_gaussian(image)));
    case F_LAPLACIAN:
        return F_detectEdge_laplacian(image);
    case F_CANNY:
        return F_detectEdge_canny(F_decolor(F_blur_gaussian(image)));
    }
    return image;
}


/***************************************************************
 * 7. 霍夫变换
****************************************************************/
// 检测直线和圆

/***************************************************************
 * 8. 二值数学形态学（结构元允许用户自定义）
****************************************************************/
// 膨胀
QImage *F_dilation(QImage *image, U_Kernel_i kernel)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int kernelSize = kernel.size(),
        halfSize = kernelSize/2,
        r, g, b, index;

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 0;
            g = 0;
            b = 0;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    if (!kernel[i+halfSize][j+halfSize])
                        continue;
                    index = qBound(0, x+j, image->width()-1)
                            + image->width() * qBound(0, y+i, image->height()-1);
                    r = r > qRed(bits[index]) ? r : qRed(bits[index]);
                    g = g > qGreen(bits[index]) ? g : qGreen(bits[index]);
                    b = b > qBlue(bits[index]) ? b : qBlue(bits[index]);
                }
            }
            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    TIMMING_END;
    return newImage;
}

// 腐蚀
QImage *F_erosion(QImage *image, U_Kernel_i kernel)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int kernelSize = kernel.size(),
        halfSize = kernelSize/2,
        r, g, b, index;

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 255;
            g = 255;
            b = 255;
            for (int i = -halfSize; i <= halfSize; i++) {
                for (int j = -halfSize; j <= halfSize; j++) {
                    if (!kernel[i+halfSize][j+halfSize])
                        continue;
                    index = qBound(0, x+j, image->width()-1)
                            + image->width() * qBound(0, y+i, image->height()-1);
                    r = r < qRed(bits[index]) ? r : qRed(bits[index]);
                    g = g < qGreen(bits[index]) ? g : qGreen(bits[index]);
                    b = b < qBlue(bits[index]) ? b : qBlue(bits[index]);
                }
            }
            newBits[y*image->width()+x] = qRgb(r,g,b);
        }
    }
    TIMMING_END;
    return newImage;
}

// 开操作
QImage *F_open(QImage *image, U_Kernel_i kernel)
{
    return F_dilation(F_erosion(image, kernel), kernel);
}

// 闭操作
QImage *F_close(QImage *image, U_Kernel_i kernel)
{
    return F_erosion(F_dilation(image, kernel), kernel);
}

QImage *F_complement(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height();
    int r;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = 255 - qRed(bits[y*width+x]);
            newBits[y*width+x] = qRgb(r, r, r);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_union(QImage *image1, QImage *image2)
{
    TIMMING_BEGIN;
    int width1 = image1->width(), height1 = image1->height(),
        width2 = image2->width(), height2 = image2->height(),
        width = max(width1, width2),
        height = max(height1, height2),
        r;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits1 = (QRgb*)image1->constBits(),
         *bits2 = (QRgb*)image2->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if ((U_legal(width1, height1, x, y) && qRed(bits1[y*width1+x])) ||
                (U_legal(width2, height2, x, y) && qRed(bits2[y*width2+x])))
                r = 255;
            else
                r = 0;

            newBits[y*width+x] = qRgb(r, r, r);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_intersection(QImage *image1, QImage *image2)
{
    TIMMING_BEGIN;
    int width1 = image1->width(), height1 = image1->height(),
        width2 = image2->width(), height2 = image2->height(),
        width = max(width1, width2),
        height = max(height1, height2),
        r;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits1 = (QRgb*)image1->constBits(),
         *bits2 = (QRgb*)image2->constBits(),
         *newBits = (QRgb*)newImage->bits();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if ((!U_legal(width1, height1, x, y) || qRed(bits1[y*width1+x])) &&
                (!U_legal(width2, height2, x, y) || qRed(bits2[y*width2+x])))
                r = 255;
            else
                r = 0;

            newBits[y*width+x] = qRgb(r, r, r);
        }
    }

    TIMMING_END;
    return newImage;
}

QImage *F_hitAndMiss(QImage *image, U_Kernel_i kernel)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int kernelSize = kernel.size(),
        halfSize = kernelSize/2,
        r, index;

    for (int x = 0; x < image->width(); x++) {
        for (int y = 0; y < image->height(); y++) {
            r = 255;
            bool same = true;
            for (int i = -halfSize; i <= halfSize && same; i++) {
                for (int j = -halfSize; j <= halfSize && same; j++) {
                    if (kernel[i+halfSize][j+halfSize]==2)
                        continue;
                    index = qBound(0, x+j, image->width()-1)
                            + image->width() * qBound(0, y+i, image->height()-1);
                    if (kernel[i+halfSize][j+halfSize]==1 && qRed(bits[index]) == 255)
                        continue;
                    if (kernel[i+halfSize][j+halfSize]==0 && qRed(bits[index]) ==0)
                        continue;

                    same = false;
                    r = 0;
                }
            }
            newBits[y*image->width()+x] = qRgb(r, r, r);
        }
    }
    TIMMING_END;
    return newImage;
}
// 细化
// [todo] don't work
QImage *F_thinning(QImage *image, U_Kernel_i kernel)
{
    return F_minus(image, F_hitAndMiss(image, kernel));
}
// 粗化
QImage *F_thickening(QImage *image, U_Kernel_i kernel)
{
    return F_union(image, F_hitAndMiss(image, kernel));
}
// 优化显示
QImage *F_contrastStretch(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), max = 0, r;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (qRed(bits[y*width+x])>max)
                max = qRed(bits[y*width+x]);
        }
    }

    if (!max)
        return newImage;

    int multiple = (int)(255.0/max);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = bits[y*width+x] * multiple;
            newBits[y*width+x] = qRgb(r, r, r);
        }
    }

    TIMMING_END;
    return newImage;
}
// 距离变换
QImage *F_distance(QImage *image)
{
    TIMMING_BEGIN;
//    F_Kernel_i kernel = U_getFlatKernel_i(3);
    U_Kernel_i kernel = {{0, 1, 0},
                         {1, 1, 1},
                         {0, 1, 0}};
    QImage *newImage = F_NEW_IMAGE(image),
           *retImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits(),
         *retBits = (QRgb *)retImage->bits();
    int kernelSize = kernel.size(),
        halfSize = kernelSize/2,
        r, distance = 1, width = image->width(), height = image->height();
    bool changed = true;

    while (changed)
    {
        changed = false;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (!qRed(bits[y*width+x])) {
                    newBits[y*width+x] = qRgb(0, 0, 0);
                    if (distance == 1) {
                        retBits[y*width+x] = qRgb(0, 0, 0);
                    }
                    continue;
                }
                r = 255;
                for (int i = -halfSize; i <= halfSize && r; i++) {
                    for (int j = -halfSize; j <= halfSize; j++) {
                        //if (!kernel[i+halfSize][j+halfSize])
                        //    continue;
                        if (U_legal(width, height, x+j, y+i) && !qRed(bits[x+j+width*(y+i)])) {
                            changed = true;
                            r = 0;
                            retBits[y*width+x] = qRgb(distance, distance, distance);
                            break;
                        }
                    }
                }
                newBits[y*width+x] = qRgb(r, r, r);
            }
        }

        if (distance>1)
            free(image);
        distance++;
        image = newImage;
        bits = (QRgb *)image->constBits();
        newImage = F_NEW_IMAGE(image);
        newBits = (QRgb *)newImage->bits();
    }
    TIMMING_END;
    return retImage;
}

// 骨架、骨架重构
QImage *F_skeletonize(QImage *image)
{
    TIMMING_BEGIN;
    QImage *disimage = F_distance(image),
           *newImage = F_NEW_IMAGE(disimage);
    QRgb *bits = (QRgb *)disimage->constBits(),
         *newBits = (QRgb*)newImage->bits();
    int width = newImage->width(), height = newImage->height();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (!qRed(bits[y*width+x]) || x==0 || x==width-1 || y==0 || y==height-1) {
                newBits[y*width+x] = qRgb(0, 0, 0);
                continue;
            }
            int index = y*width+x,
                color00 = qRed(bits[index-width-1]),
                color01 = qRed(bits[index-width]),
                color02 = qRed(bits[index-width+1]),
                color10 = qRed(bits[index-1]),
                color11 = qRed(bits[index]),
                color12 = qRed(bits[index+1]),
                color20 = qRed(bits[index+width-1]),
                color21 = qRed(bits[index+width]),
                color22 = qRed(bits[index+width+1]),
                count = 0;

            if (color11>=color00)
                count++;
            if (color11>=color01)
                count++;
            if (color11>=color02)
                count++;
            if (color11>=color10)
                count++;
            if (color11>=color12)
                count++;
            if (color11>=color20)
                count++;
            if (color11>=color21)
                count++;
            if (color11>=color22)
                count++;

            if (count>7) {
                newBits[y*width+x] = qRgb(color11, color11, color11);
            }
            else
                newBits[y*width+x] = qRgb(0, 0, 0);
        }
    }

    free(disimage);
    TIMMING_END;
    return newImage;
}

QImage *F_contrastRecover(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), minVal = 255, r;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (qRed(bits[y*width+x]) && qRed(bits[y*width+x]) < minVal)
                minVal = qRed(bits[y*width+x]);
        }
    }

    int multiple = minVal;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            r = qRed(bits[y*width+x]) / multiple;
            newBits[y*width+x] = qRgb(r, r, r);
        }
    }

    TIMMING_END;
    qDebug()<<"contrast recover complete, muptiple = "<<multiple;
    return newImage;
}

QImage *F_skeletonReconstruct(QImage *image)
{
    TIMMING_BEGIN;
    QImage *tmpImage = image,
           *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)tmpImage->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), color;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            newBits[y*width+x] = qRgb(0, 0, 0);
        }
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            color = qRed(bits[y*width+x]) - 1;
            if (color>200)
                return image;
            if (color >= 0) {
                for (int i = -color; i <= color; i++)
                {
                    for (int j = -color; j <= color; j++)
                    {
                        if (!U_legal(width, height, x+j, y+i)) {
                            //qDebug()<<"error"<<x<<y<<i<<j;
                            continue;
                        }
                        newBits[(y+i)*width+x+j] = qRgb(255, 255, 255);
                    }
                }
            }
        }
    }

    free(tmpImage);
    TIMMING_END;
    return newImage;
}
// 二值形态学重构
QImage *F_reconstruct(QImage *marker, QImage *mask)
{
    TIMMING_BEGIN;
    Q_ASSERT(marker->size() == mask->size());

    QImage *newImage = new QImage(*marker);
    QRgb *maskBits = (QRgb*)mask->constBits(),
         *newBits = (QRgb *)newImage->bits();
    U_Kernel_i kernel = {{0, 1, 0},
                         {1, 1, 1},
                         {0, 1, 0}};
    int halfSize = kernel.size()/2,
        width = marker->width(), height = marker->height(),
        index, r, rr;
    bool changed = true;

    while (changed) {
        changed = false;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                index = y*width+x;
                r = qRed(newBits[index]);
                if (!r)
                    continue;

                for (int i = -halfSize; i <= halfSize; i++) {
                    for (int j = -halfSize; j <= halfSize; j++) {
                        if (!kernel[i+halfSize][j+halfSize] ||
                            !U_legal(width, height, x+j+halfSize, y+i+halfSize))
                            continue;
                        index = (y+i)*width + x+j;
                        rr = min(r, qRed(maskBits[index]));
                        if (qRed(newBits[index]) < rr) {
                            changed = true;
                            newBits[index] = qRgb(rr, rr, rr);
                        }
                    }
                }
            }
        }
    }
    TIMMING_END;
    return newImage;
}

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 分水岭算法
class WatershedPoint {
public:
    int x, y, intensity;
    int *label = NULL;
    bool isShed = false;
    vector<WatershedPoint *> neighbors;

    WatershedPoint(int _x, int _y, int _intensity)
        :x(_x), y(_y), intensity(_intensity)
    {
    }

    bool neighborsNotAllShed()
    {
        for (WatershedPoint *p:neighbors) {
            if (!(p->isShed))
                return true;
        }
        return false;
    }
};

void addNeighbor(WatershedPoint *p1, WatershedPoint *p2)
{
    p1->neighbors.push_back(p2);
    p2->neighbors.push_back(p1);
}

void addQueue(queue<WatershedPoint *> &q, vector<WatershedPoint *> &v)
{
    for (WatershedPoint *p:v) {
        q.push(p);
    }
}

#define F_WATERSHED_TICK 20
QImage *F_watershed(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = new QImage(*image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    int width = image->width(), height = image->height(), newLabel = 1;

    // 1. sort pixel by intensity
    vector<WatershedPoint *> points;
    for (int y = 0; y<height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            points.push_back(new WatershedPoint(x, y, qRed(bits[y*width+x])));
            if (U_legal(width, height, x-1, y)) {
                addNeighbor(points[points.size()-1], points[points.size()-2]);
            }
            if (U_legal(width, height, x, y-1)) {
                addNeighbor(points[points.size()-1], points[points.size()-1-width]);
            }
            if (U_legal(width, height, x-1, y-1)) {
                addNeighbor(points[points.size()-1], points[points.size()-2-width]);
            }
        }
    }
    sort(points.begin(), points.end(), [](const WatershedPoint *p1, const WatershedPoint *p2)
    {
       return p1->intensity<p2->intensity;
    });

    // 2. label
    int threshold = F_WATERSHED_TICK;

    for (WatershedPoint *point:points)
    {
        if (point->intensity >= threshold)
            threshold = point->intensity + F_WATERSHED_TICK - point->intensity % F_WATERSHED_TICK;

        if (point->label) {
            //qDebug()<<"\tcolored";
            continue;
        }

        for (WatershedPoint *neighbor:point->neighbors)
        {
            if (neighbor->label) {
                point->label = neighbor->label;
                //qDebug()<<"\tfind neighbor color"<<neighbor->label;
                break;
            }
        }

        if (point->label)
            continue;
        point->label = (int*)malloc(sizeof(int));
        *(point->label) = newLabel++;
        //qDebug()<<"("<<point->x<<","<<point->y<<"):"<<point->intensity<<"color"<<*(point->label);

        queue<WatershedPoint *> scanQueue;
        addQueue(scanQueue, point->neighbors);
        bool isMerged = false;

        while (scanQueue.size())
        {
            WatershedPoint *head = scanQueue.front();
            //qDebug()<<scanQueue.size()<<","<<head->x<<head->y;
            scanQueue.pop();
            if (head->label) {
                if (*(head->label) != *(point->label)) {
                    if (isMerged) {
                        head->isShed = true;
                        //qDebug()<<"Shed: "<<"("<<head->x<<","<<head->y<<")";
                    } else {
                        //qDebug()<<"\tPoint"<<point->x<<point->y<<head->x<<head->y;
                        //qDebug()<<"\t\tColor"<<*(point->label)<<"merge with"<<*(head->label);
                        isMerged = true;
                        *(point->label) = *(head->label);
                    }
                }
                continue;
            }
            if (head->intensity - point->intensity < F_WATERSHED_TICK && head->intensity>=point->intensity) {
                head->label = point->label;
                addQueue(scanQueue, head->neighbors);
            }
        }
    }

    // 3. color
    for (WatershedPoint *point:points)
    {
        if (point->isShed && point->neighborsNotAllShed())
        {
            newBits[point->y*width+point->x] = qRgb(255, 0, 0);
        }
    }
    TIMMING_END;
    return newImage;
}

/***************************************************************
 * 10. 霍夫变换
****************************************************************/
#define HOUGH_TEST1

QImage *F_hough_line(QImage *image, int hough_limit)
{
    TIMMING_BEGIN;
    int width = image->width(), height = image->height();
    double rho_max = sqrt(width*width+height*height), theta_max = PI/2.0;
    int rho_size = 180, theta_size = 180;
    double rho_accuracy = rho_max / rho_size,
           theta_accuracy = theta_max / theta_size;

#ifndef HOUGH_TEST
    QImage *newImage = new QImage(*image);
#else
    QImage *newImage = new QImage(theta_size, rho_size, QImage::Format_ARGB32);
#endif
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    vector<vector<int>> houghSpace;
    for (int i=0; i< theta_size; i++) {
        vector<int> line(rho_size, 0);
        houghSpace.push_back(line);
    }

    for (int y = 1; y < height; y++)
    {
        for (int x = 1; x < width; x++)
        {
            if (qRed(bits[y*width+x]) == 255)
            {
                for (int i = 0; i < theta_size; i++) {
                    int j = U_round((x*cos(i*theta_accuracy)+y*(sin(i*theta_accuracy)))/rho_accuracy);
                    //qDebug()<<"["<<theta<<","<<rho<<"]";
                    if (j < rho_size) {
                        houghSpace[i][j]++;
                    }
                }
            }
        }
    }

#ifdef HOUGH_TEST
    for (int i = 0; i < theta_size; i++) {
        for (int j = 0; j < rho_size; j++) {
            int color = qBound(0, houghSpace[i][j], 255);
            newBits[i*rho_size+j] = qRgb(color, color, color);
        }
    }
#else
    for (int i = 0; i < theta_size; i++) {
        for (int j = 0; j < rho_size; j++) {
            double theta = i * theta_accuracy,
                   rho = j * rho_accuracy;

            if (houghSpace[i][j] > hough_limit) {
                if (theta < PI/4) {
                    for (int y = 0; y < height; y++) {
                        int x = U_round((rho - y * sin(theta))/cos(theta));
                        if (U_legal(width, height, x, y) && qRed(bits[y*width+x]))
                            newBits[y*width+x] = qRgb(255, 0, 0);
                    }
                } else {
                    for (int x = 0; x < width; x++) {
                        int y = U_round((rho - x * cos(theta))/sin(theta));
                        if (U_legal(width, height, x, y) && qRed(bits[y*width+x]))
                            newBits[y*width+x] = qRgb(255, 0, 0);
                    }
                }
            }
        }
    }
#endif
    TIMMING_END;
    return newImage;
}
