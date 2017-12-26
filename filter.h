#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <Qimage>
#include <QMainWindow>
#include <QRgb>
#include <QDebug>

#include <iostream>
#include <time.h>
#include <vector>

// color channel for function F_seperation()
enum F_Channel{F_R, F_G, F_B};
enum F_ScaleAlgo{F_NEAREST, F_LINEAR};

struct F_HSI
{
    float h, s, i;
};

double F_responseTime();

/***************************************************************
 * 1. 彩色图像处理
****************************************************************/
// 三通道分离
QImage *F_seperation(QImage *image, F_Channel channel);

// 彩色->灰度
QImage *F_decolor(QImage *image);

// 色相/饱和度/亮度调节
F_HSI F_RGB2HSI(QRgb rgb);
QRgb F_HSI2RGB(F_HSI hsi);
QImage *F_changeHSI(QImage *image);

// 色阶调整(选做)

/***************************************************************
 * 2. 二值化
****************************************************************/
QImage *F_binarization(QImage *image, int threshold);
// Otus(大津算法)
// 手动调节：双阈值，实时

/***************************************************************
 * 3. 代数与几何操作
****************************************************************/
// 加、减、乘、剪裁

// 缩放、旋转（至少两种差值方式：最近邻、双线性）
// TODO
QImage *F_resize_linear(QImage *image, int width, int height);
QImage *F_resize_nearest(QImage *image, int width, int height);
QImage *F_resize(QImage *image, F_ScaleAlgo algo);
QImage *F_spin(QImage *image, int angle);

/***************************************************************
 * 4. 对比度调节
****************************************************************/
// 线性及分段线性调整
// 非线性调整：对数、指数（系数可调）

// 图像的直方图显示
std::vector<int> F_getHistogram(QImage *image);
void F_getHistogram_rgb(QImage *image, std::vector<int> &r_h, std::vector<int> &g_h, std::vector<int> &b_h);

// 直方图均衡化
QImage *F_equalizeHistogram(QImage *image);

/***************************************************************
 * 5. 平滑滤波器（卷积核允许用户自定义）
****************************************************************/
// 均值、中值、高斯
QImage *F_convolution(QImage *image, int kernel[], int kernelSize, int kernelSum);
QImage *F_blur(QImage *image);
QImage *F_sharpen(QImage *image);

/***************************************************************
 * 6. 边缘检测
****************************************************************/
// Sobel、拉普拉斯、canny


/***************************************************************
 * 7. 霍夫变换（选做）
****************************************************************/
// 检测直线和圆


/***************************************************************
 * 8. 二值数学形态学（结构元允许用户自定义）
****************************************************************/
// 膨胀、腐蚀、开、闭、细化、粗化、距离变换、骨架、骨架重构、二值形态学重构
QImage *F_dilation(QImage *image);
QImage *F_erosion(QImage *image);
QImage *F_open(QImage *image);
QImage *F_close(QImage *image);

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 膨胀、腐蚀、开、闭、形态学重构、分水岭算法

#endif // PROCESSOR_H
