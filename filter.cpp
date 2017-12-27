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
            int avg = (qRed(bits[index]) + qGreen(bits[index]) + qBlue(bits[index]))/3;
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
    float hue = hsi.h, saturation = hsi.s, intensity = hsi.i;
    float r, g, b;

    float pi = 3.1415926;
    float otz = 2*pi / 3;
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

    float rate_w = float(oldWidth - 1) / (width - 1),
          rate_h = float(oldHeight - 1) / (height - 1),
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
                    r = left_y * qRed(bits[index]) + right_y * qRed(bits[index + oldWidth]);
                    g = left_y * qGreen(bits[index]) + right_y * qGreen(bits[index + oldWidth]);
                    b = left_y * qBlue(bits[index]) + right_y * qBlue(bits[index + oldWidth]);
                }
            } else {
                if (y == height - 1) {
                    r = left_x * qRed(bits[index]) + right_x * qRed(bits[index + 1]);
                    g = left_x * qGreen(bits[index]) + right_x * qGreen(bits[index + 1]);
                    b = left_x * qBlue(bits[index]) + right_x * qBlue(bits[index + 1]);
                } else {
                    r = left_y * (left_x * qRed(bits[index]) + right_x * qRed(bits[index + 1]))
                            + right_y * (left_x * qRed(bits[index + oldWidth]) + right_x * qRed(bits[index + oldWidth + 1]));
                    g = left_y * (left_x * qGreen(bits[index]) + right_x * qGreen(bits[index + 1]))
                            + right_y * (left_x * qGreen(bits[index + oldWidth]) + right_x * qGreen(bits[index + oldWidth + 1]));
                    b = left_y * (left_x * qBlue(bits[index]) + right_x * qBlue(bits[index + 1]))
                            + right_y * (left_x * qBlue(bits[index + oldWidth]) + right_x * qBlue(bits[index + oldWidth + 1]));
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

    float rate_w = float(oldWidth - 1) / (width - 1),
          rate_h = float(oldHeight - 1) / (height - 1);

    for (int y = 0; y < height; y++)
    {
        target_h = int(y * rate_h + 0.499);
        for (int x = 0; x < width; x++)
        {
            target_w = int(x* rate_w + 0.499);
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
    float theta = angle/180.0*3.14159265;
    int width = image->width(), height = image->height(),
        newWidth = ceil(abs(width*cos(theta)) + abs(height*sin(theta))),
        newHeight = ceil(abs(height*cos(theta)) + abs(width*sin(theta))),
        index, target_index,
        target_x, target_y,
        left, right, top, bottom,
        r, g, b;
    float decimal_x, decimal_y,
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
    float theta = angle/180.0*3.14159265;
    int width = image->width(), height = image->height(),
        newWidth = ceil(abs(width*cos(theta)) + abs(height*sin(theta))),
        newHeight = ceil(abs(height*cos(theta)) + abs(width*sin(theta))),
        index, target_index,
        target_x, target_y;
    float decimal_x, decimal_y,
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

            target_x = (int)(decimal_x + 0.499);
            target_y = (int)(decimal_y + 0.499);

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
vector<int> F_getHistogram(QImage *image)
{
    vector<int> histogram(256);
    QRgb *bits = (QRgb *)image->constBits();
    int index,
        width = image->width(),
        height = image->height();

    for (int x=0; x<width; x++)
    {
        for (int y=0; y<height; y++)
        {
            index = y*image->width()+x;
            int avg = (qRed(bits[index]) + qGreen(bits[index]) + qBlue(bits[index]))/3;
            histogram[avg]++;
        }
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
// [test] done [src] internet
QImage *F_equalizeHistogram(QImage *image)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();
    // rgb histogram
    vector<int> r_h(256), g_h(256), b_h(256);
    // rgb proportion
    vector<float> r_pro(256), g_pro(256), b_pro(256);
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
// 均值、中值、高斯
QImage *F_convolution(QImage *image, int kernel[], int kernelSize, int kernelSum)
{
    TIMMING_BEGIN;
    int halfSize = kernelSize/2,
        r, g, b, bitsIndex, kernelIndex;
    QImage *newImage = F_NEW_IMAGE(image);
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
    TIMMING_END;
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

/***************************************************************
 * 6. 边缘检测
****************************************************************/
// Sobel、拉普拉斯、canny


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
    TIMMING_END;
    return newImage;
}

// 开操作
QImage *F_open(QImage *image)
{
    return NULL;
}

// 闭操作
QImage *F_close(QImage *image)
{
    return NULL;
}

// 细化、粗化、距离变换、骨架、骨架重构、二值形态学重构

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 膨胀、腐蚀、开、闭、形态学重构、分水岭算法
