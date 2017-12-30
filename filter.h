#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <Qimage>
#include <QMainWindow>
#include <QRgb>
#include <QDebug>

#include <iostream>
#include <time.h>
#include <vector>
#include <stack>

#include "util.h"

// color channel for function F_seperation()
enum F_Channel{F_R, F_G, F_B};
enum F_ScaleAlgo{F_NEAREST, F_LINEAR};
enum F_DetectEdgeAlgo{F_SOBEL, F_LAPLACIAN, F_CANNY};

struct F_HSI
{
    double h, s, i;
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
QImage *F_binarization_Otsu(QImage *image);
// 手动调节：双阈值，实时
QImage *F_binarization_double(QImage *image, int threshold_low, int threshold_high);

/***************************************************************
 * 3. 代数与几何操作
****************************************************************/
// 加、减、乘、剪裁
QImage *F_add(QImage *image1, QImage *image2);
QImage *F_minus(QImage *image1, QImage *image2);
QImage *F_times(QImage *image1, QImage *image2);
QImage *F_cut(QImage *image, int x_start, int y_start, int width, int height);

// 缩放（最近邻、双线性）
QImage *F_resize_linear(QImage *image, int width, int height);
QImage *F_resize_nearest(QImage *image, int width, int height);
QImage *F_resize(QImage *image, F_ScaleAlgo algo);

// 旋转（最近邻、双线性）
QImage *F_cut_transparent(QImage *image);
QImage *F_spin_linear(QImage *image, int angle);
QImage *F_spin_nearest(QImage *image, int angle);
QImage *F_spin(QImage *image, int angle, F_ScaleAlgo algo);

/***************************************************************
 * 4. 对比度调节
****************************************************************/
// 线性及分段线性调整
// 非线性调整：对数、指数（系数可调）

// 图像的直方图显示
std::vector<double> F_getHistogram(QImage *image);
void F_getHistogram_rgb(QImage *image, std::vector<int> &r_h, std::vector<int> &g_h, std::vector<int> &b_h);

// 直方图均衡化
QImage *F_equalizeHistogram(QImage *image);

/***************************************************************
 * 5. 平滑滤波器（卷积核允许用户自定义）
****************************************************************/
QImage *F_convolution(QImage *image, std::vector<std::vector<int>> kernel, int kernelSum);
// 均值、中值、高斯、锐化（自做）
QImage *F_blur_mean(QImage *image, int radius);
QImage *F_blur_median(QImage *image, int radius);
QImage *F_blur_gaussian(QImage *image);
QImage *F_sharpen(QImage *image);

/***************************************************************
 * 6. 边缘检测
****************************************************************/
// Sobel、拉普拉斯、canny
QImage *F_detectEdge_sobel(QImage *image);
QImage *F_detectEdge_laplacian(QImage *image);
QImage *F_detectEdge_canny(QImage *image);
QImage *F_detectEdge(QImage *image, F_DetectEdgeAlgo algo);

/***************************************************************
 * 7. 霍夫变换（选做）
****************************************************************/
// 检测直线和圆


/***************************************************************
 * 8. 二值数学形态学（结构元允许用户自定义）
****************************************************************/
// 膨胀、腐蚀、开、闭
QImage *F_dilation(QImage *image);
QImage *F_erosion(QImage *image);
QImage *F_open(QImage *image);
QImage *F_close(QImage *image);
// 细化、粗化
// 距离变换
// 骨架、骨架重构
// 二值形态学重构

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 膨胀、腐蚀、开、闭、形态学重构、分水岭算法

#endif // PROCESSOR_H
