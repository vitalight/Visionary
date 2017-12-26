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
// [test] none
QImage *F_seperation(QImage *image, F_Channel channel)
{
    TIMMING_BEGIN;
    QImage *newImage = F_NEW_IMAGE(image);
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
F_HSI F_RGB2HSI(QRgb rgb)
{
    // is rgb scaled to 1?

    return F_HSI{1, 1, 1};
}

// [test] none
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

// 缩放（至少两种差值方式：最近邻、双线性）
QImage *F_resize_linear(QImage *image, int width, int height)
{
    TIMMING_BEGIN;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    int oldWidth = image->width(),
        oldHeight = image->height(),
        target_w, target_h;

    float rate_w = float(oldWidth) / width,
          rate_h = float(oldHeight) / height;

    for (int y = 0; y < height; y++)
    {
        target_h = (y+1) * rate_h - 1;
        for (int x = 0; x < width; x++)
        {
            target_w = (x+1) * rate_w - 1;
            newBits[x + y * width] = bits[target_w + target_h * oldWidth];
        }
    }
    TIMMING_END;
    return newImage;
#if 0
    int width,height;

    width=image->width();
    height=image->height();
    double x,y,r1,r2,g1,g2,b1,b2;
    int i,j;
    double width_bi,height_bi;
    QRgb rgb00,rgb01,rgb10,rgb11;
    int r,g,b;
    QImage SouImage;
    width_bi=setWidthBi->value();
    height_bi=setHeightBi->value();
    SouImage=QImage(width*width_bi,height*height_bi,QImage::Format_ARGB32);
    for(i=0;i<width*width_bi;i++)
    {
        for(j=0;j<height*height_bi;j++)
        {
            x=i*(1/width_bi);
            y=j*(1/height_bi);

            //边界采用单线性插值
            if(ceil(x)==0&&ceil(y)!=0)
            {
                rgb00=image->pixel(0,ceil(y)-1);
                rgb01=image->pixel(0,ceil(y));
                r=(ceil(y)-y)*qRed(rgb00)+(y-(ceil(y)-1))*qRed(rgb01);
                g=(ceil(y)-y)*qGreen(rgb00)+(y-(ceil(y)-1))*qGreen(rgb01);
                b=(ceil(y)-y)*qBlue(rgb00)+(y-(ceil(y)-1))*qBlue(rgb01);
                SouImage.setPixel(i,j,qRgb(r,g,b));
            }

            if(ceil(y)==0&&ceil(y)!=0)
            {
                rgb00=image->pixel(ceil(x)-1,0);
                rgb10=image->pixel(ceil(x),0);
                r=(ceil(x)-x)*qRed(rgb00)+(x-(ceil(x)-1))*qRed(rgb10);
                g=(ceil(x)-x)*qGreen(rgb00)+(x-(ceil(x)-1))*qGreen(rgb10);
                b=(ceil(x)-x)*qBlue(rgb00)+(x-(ceil(x)-1))*qBlue(rgb10);
                SouImage.setPixel(i,j,qRgb(r,g,b));
            }

            //(0,0)点特殊处理
            if(ceil(y)==0&&ceil(y)==0)
            {
                rgb00=image->pixel(0,0);
                SouImage.setPixel(i,j,rgb00);
            }

            //非边界采用双线性插值
            if(ceil(x)!=0&&ceil(y)!=0)
            {
                rgb00=image->pixel(ceil(x)-1,ceil(y)-1);
                rgb01=image->pixel(ceil(x)-1,ceil(y));
                rgb10=image->pixel(ceil(x),ceil(y)-1);
                rgb11=image->pixel(ceil(x),ceil(y));

                r1=(ceil(x)-x)*qRed(rgb00)+(x-(ceil(x)-1))*qRed(rgb10);
                r2=(ceil(x)-x)*qRed(rgb01)+(x-(ceil(x)-1))*qRed(rgb11);
                r=(int)((ceil(y)-y)*r1+(y-(ceil(y)-1))*r2);

                g1=(ceil(x)-x)*qGreen(rgb00)+(x-(ceil(x)-1))*qGreen(rgb10);
                g2=(ceil(x)-x)*qGreen(rgb01)+(x-(ceil(x)-1))*qGreen(rgb11);
                g=(int)((ceil(y)-y)*g1+(y-(ceil(y)-1))*g2);

                b1=(ceil(x)-x)*qBlue(rgb00)+(x-(ceil(x)-1))*qBlue(rgb10);
                b2=(ceil(x)-x)*qBlue(rgb01)+(x-(ceil(x)-1))*qBlue(rgb11);
                b=(int)((ceil(y)-y)*b1+(y-(ceil(y)-1))*b2);

                SouImage.setPixel(i,j,qRgb(r,g,b));
            }
        }
    }
    ui->label_2->resize(SouImage.width(),SouImage.height());
    ui->label_2->setPixmap(QPixmap::fromImage(SouImage));
#endif
}

QImage *F_resize_nearest(QImage *image, int width, int height)
{
    TIMMING_BEGIN;
    QImage *newImage = new QImage(width, height, QImage::Format_ARGB32);
    QRgb *bits = (QRgb *)image->constBits(),
         *newBits = (QRgb *)newImage->bits();

    int oldWidth = image->width(),
        oldHeight = image->height(),
        target_w, target_h;

    float rate_w = float(oldWidth) / width,
          rate_h = float(oldHeight) / height;

    for (int y = 0; y < height; y++)
    {
        target_h = (y+1) * rate_h - 1;
        for (int x = 0; x < width; x++)
        {
            target_w = (x+1) * rate_w - 1;
            newBits[x + y * width] = bits[target_w + target_h * oldWidth];
        }
    }
    TIMMING_END;
    return newImage;
}

QImage *F_resize(QImage *image, F_ScaleAlgo algo)
{
    int width = 500, height = 1000;

    if (algo == F_NEAREST)
        return F_resize_nearest(image, width, height);
    else
        return F_resize_linear(image, width, height);
}

// 旋转
// [todo]
QImage *F_spin(QImage *image, int angle)
{
    return NULL;
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
