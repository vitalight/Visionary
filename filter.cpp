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
// [test] none
#if 0
F_HSI F_RGB2HSI(QRgb rgb)
{
    // is rgb scaled to 1?
    return F_HSI{1, 1, 1};
}
#endif

// [test] none
#if 0
QRgb F_HSI2RGB(F_HSI hsi)
{
    double hue = hsi.h, saturation = hsi.s, intensity = hsi.i;
    double r, g, b;

    double pi = PI;
    double otz = 2*pi / 3;
    if (hue>=0&&hue<=otz)
    {
        r = intensity*(1+(saturation*cos(hue))/(cos(pi/3.0-hue)));
        b = intensity*(1-saturation);
        g = 3*intensity - (b+r);
    }
    else if (hue>=otz && hue<2*otz)
    {
        r = intensity*(1-saturation);
        g = intensity*(1+(saturation*cos(hue-otz))/(cos(pi-hue)));
        b = 3*intensity-(r+g);
    }
    else
    {
        g = intensity*(1-saturation);
        b = intensity*(1+(saturation*cos(hue-otz*2))/(cos(5*pi/6-hue)));
        r = 3*intensity-(g+b);
    }
    return qRgb(r, g, b);
}
#endif

QImage *F_changeHSI(QImage *image);

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

    qDebug()<<"Threshold: "<<threshold;
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
// 加、减、乘、剪裁

// 双线性插值缩放
// [test] required
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
        target_h = (int)decimal_h;
        left_y = decimal_h - target_h;
        right_y = 1 - left_y;
        for (int x = 0; x < width; x++)
        {
            decimal_w = x * rate_w;
            target_w = (int)decimal_w;
            left_x = decimal_w - target_w;
            right_x = 1 - left_x;

            index = target_w + target_h * oldWidth;

            if (x == width - 1) {
                if (y == height - 1) {
                    r = qRed(bits[index]);
                    g = qGreen(bits[index]);
                    b = qBlue(bits[index]);
                } else {
                    r = left_y * qRed(bits[index]) * qAlpha(bits[index]) + right_y * qRed(bits[index + oldWidth]) * qAlpha(bits[index+oldWidth]);
                    g = left_y * qGreen(bits[index]) * qAlpha(bits[index]) + right_y * qGreen(bits[index + oldWidth]) * qAlpha(bits[index+oldWidth]);
                    b = left_y * qBlue(bits[index]) * qAlpha(bits[index]) + right_y * qBlue(bits[index + oldWidth]) * qAlpha(bits[index+oldWidth]);
                }
            } else {
                if (y == height - 1 || !qAlpha(bits[index])) {
                    r = left_x * qRed(bits[index]) * qAlpha(bits[index]) + right_x * qRed(bits[index + 1]) * qAlpha(bits[index+1]);
                    g = left_x * qGreen(bits[index]) * qAlpha(bits[index]) + right_x * qGreen(bits[index + 1]) * qAlpha(bits[index+1]);
                    b = left_x * qBlue(bits[index]) * qAlpha(bits[index]) + right_x * qBlue(bits[index + 1]) * qAlpha(bits[index+1]);
                } else {
                    r = left_y * (left_x * qRed(bits[index]) * qAlpha(bits[index]) + right_x * qRed(bits[index + 1]))
                            + right_y * (left_x * qRed(bits[index + oldWidth]) * qAlpha(bits[index]) + right_x * qRed(bits[index + oldWidth + 1])) * qAlpha(bits[index+oldWidth+1]);
                    g = left_y * (left_x * qGreen(bits[index]) * qAlpha(bits[index]) + right_x * qGreen(bits[index + 1]))
                            + right_y * (left_x * qGreen(bits[index + oldWidth]) * qAlpha(bits[index]) + right_x * qGreen(bits[index + oldWidth + 1])) * qAlpha(bits[index+oldWidth+1]);
                    b = left_y * (left_x * qBlue(bits[index]) + right_x * qBlue(bits[index + 1]))
                            + right_y * (left_x * qBlue(bits[index + oldWidth]) * qAlpha(bits[index]) + right_x * qBlue(bits[index + oldWidth + 1])) * qAlpha(bits[index+oldWidth+1]);
                }
            }

            newBits[x + y * width] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return newImage;
}

// 最近邻缩放
// [test] required
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

QImage *F_resize(QImage *image, F_ScaleAlgo algo)
{
    int width = 400, height = 200;

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
    //qDebug()<<min_x<<", "<<max_x<<", "<<min_y<<", "<<max_y;
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
// [improve] border missing dots
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
            // todo
            decimal_x = x*cos(theta) + y*sin(theta) + delta_x;
            decimal_y = -x*sin(theta) + y*cos(theta) + delta_y;

            target_x = (int)decimal_x;
            target_y = (int)decimal_y;

            left = decimal_x - target_x;
            right = 1-left;
            top = decimal_y - target_y;
            bottom = 1-top;

            // ignore blank space
//            if (decimal_x < 0 || decimal_x >= width-1 || decimal_y < 0 || decimal_y >= height-1) {
//                continue;
//            }
            index = y * newWidth + x;
            target_index = target_x + target_y * width;

            if (target_x < 0 || target_x >= width-1 ||
                target_y < 0 || target_y >= height-1 ||
                !qAlpha(bits[target_index])) {
                continue;
            }


            if (decimal_x == width) {
                if (decimal_y == height) {

                }
            } else {
                if (decimal_y == height) {

                }
            }

            r = left * (top * qRed(bits[target_index]) + bottom * qRed(bits[target_index+width]))
                + right * (top * qRed(bits[target_index+1]) + bottom * qRed(bits[target_index+width+1]));
            g = left * (top * qGreen(bits[target_index]) + bottom * qGreen(bits[target_index+width]))
                + right * (top * qGreen(bits[target_index+1]) + bottom * qGreen(bits[target_index+width+1]));
            b = left * (top * qBlue(bits[target_index]) + bottom * qBlue(bits[target_index+width]))
                + right * (top * qBlue(bits[target_index+1]) + bottom * qBlue(bits[target_index+width+1]));

            newBits[index] = qRgb(r, g, b);
        }
    }

    TIMMING_END;
    return F_cut_transparent(newImage);
}

// 最近邻旋转
// [improve] border missing dots
QImage *F_spin_nearest(QImage *image, int angle)
{
    TIMMING_BEGIN;
    double theta = angle/180.0*PI;
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

            // ignore blank space
            if (target_x < 0 || target_x >= width || target_y < 0 || target_y >= height) {
                continue;
            }

            index = y * newWidth + x;
            target_index = target_x + target_y * width;
            newBits[index] = bits[target_index];
        }
    }

    TIMMING_END;
    return F_cut_transparent(newImage);
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
// 非线性调整：对数、指数（系数可调）

// 图像的直方图显示
// [test] none
vector<double> F_getHistogram(QImage *image)
{
    vector<double> histogram(256);
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
QImage *F_convolution(QImage *image, vector<vector<double>> kernel, int kernelSum)
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
    vector<vector<double>> kernel = U_getFlatKernel_d(radius);
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
QImage *F_blur_gaussian(QImage *image)
{
    vector<vector<double>> kernel = U_getGaussianKernel(5, 1);
    return F_convolution(image, kernel, 1);
}

// 锐化（自定）
QImage *F_sharpen(QImage *image)
{
    vector<vector<double>> kernel = {{0, -1,  0},
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
    vector<vector<int>> kernel_x = {{-1, 0, 1},
                                    {-2, 0, 2},
                                    {-1, 0, 1}};
    vector<vector<int>> kernel_y = {{-1, -2, -1},
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
    vector<vector<double>> kernel = {{0, 1, 0},
                                     {1,-4, 1},
                                     {0, 1, 0}};
    return F_convolution(image, kernel, 1);
}
// canny
QImage *F_detectEdge_canny(QImage *image)
{
    TIMMING_BEGIN;
    int threshold_low = 55, threshold_high = 90;

    vector<vector<int>> kernel_x = {{-1,  0,  1},
                                    {-2,  0,  2},
                                    {-1,  0,  1}};
    vector<vector<int>> kernel_y = {{-1, -2, -1},
                                    { 0,  0,  0},
                                    { 1,  2,  1}};
    vector<vector<int>> direction;

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
                qDebug()<<"direction["<<y<<"]["<<x<<"]";
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

// [todo] blur or not ?
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
QImage *F_dilation(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    vector<vector<int>> kernel = U_getFlatKernel_i(5);
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
                    if (!kernel[i+halfSize][j+halfSize] ||
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
    TIMMING_END;
    return newImage;
}

// 腐蚀
QImage *F_erosion(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    vector<vector<int>> kernel = U_getFlatKernel_i(5);
    int kernelSize = kernel.size(),
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
                    if (!kernel[i+halfSize][j+halfSize] ||
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
    TIMMING_END;
    return newImage;
}

// 开操作
QImage *F_open(QImage *image)
{
    return F_dilation(F_erosion(image));
}

// 闭操作
QImage *F_close(QImage *image)
{
    return F_erosion(F_dilation(image));
}

// 细化
// 粗化
// 距离变换
// 骨架、骨架重构
QImage *F_skeletonize(QImage *image)
{
    return image;
}
// 二值形态学重构

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 膨胀、腐蚀、开、闭、形态学重构、分水岭算法
