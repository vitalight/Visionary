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
#include <queue>

#include "util.h"

// color channel for function F_seperation()
enum F_Channel{F_R, F_G, F_B};
enum F_ScaleAlgo{F_NEAREST, F_LINEAR};
enum F_DetectEdgeAlgo{F_SOBEL, F_LAPLACIAN, F_CANNY};

struct F_HSB
{
    double h, s, b;
    F_HSB(double h_, double s_, double b_)
        : h(h_), s(s_), b(b_)
    {
    }
};

double F_responseTime();

QImage *F_tag(QImage *image, int x, int y);
/***************************************************************
 * 1. 彩色图像处理
****************************************************************/
// 三通道分离
QImage *F_seperation(QImage *image, F_Channel channel);

// 彩色->灰度
QImage *F_decolor(QImage *image);

// 色相/饱和度/亮度调节
F_HSB F_RGB2HSB(QRgb rgb);
QRgb F_HSB2RGB(F_HSB hsi);
QImage *F_adjustHSB(QImage *image, int h_val, int s_val, int b_val);

// 色阶调整(选做)
QImage *F_colorGradation(QImage *image, int shadow, double midtone, int highlight);

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
QImage *F_resize(QImage *image, int width, int height, F_ScaleAlgo algo);

// 旋转（最近邻、双线性）
QImage *F_cut_transparent(QImage *image);
QImage *F_spin_linear(QImage *image, int angle);
QImage *F_spin_nearest(QImage *image, int angle);
QImage *F_spin(QImage *image, int angle, F_ScaleAlgo algo);

/***************************************************************
 * 4. 对比度调节
****************************************************************/
// 线性及分段线性调整
QImage *F_contrast_linear(QImage *image, double gradient, int intercept);
QImage *F_contrast_section(QImage *image, int pointX1, int pointY1, int pointX2, int pointY2);
// 非线性调整：对数、指数（系数可调）
QImage *F_contrast_logarithm(QImage *image, double arga, double argb, double argc);
QImage *F_contrast_exponential(QImage *image, double arga, double argb, double argc);

// 图像的直方图显示
std::vector<double> F_getHistogram(QImage *image);
void F_getHistogram_rgb(QImage *image, std::vector<int> &r_h, std::vector<int> &g_h, std::vector<int> &b_h);

// 直方图均衡化
QImage *F_equalizeHistogram(QImage *image);

/***************************************************************
 * 5. 平滑滤波器（卷积核允许用户自定义）
****************************************************************/
QImage *F_convolution(QImage *image, U_Kernel_d kernel, double kernelSum);
// 均值、中值、高斯、锐化（自做）
QImage *F_blur_mean(QImage *image, int radius);
QImage *F_blur_median(QImage *image, int radius);
QImage *F_blur_gaussian(QImage *image, int radius = 5, double sigma = 1.0);
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
QImage *F_dilation(QImage *image, U_Kernel_i kernel);
QImage *F_erosion(QImage *image, U_Kernel_i kernel);
QImage *F_open(QImage *image, U_Kernel_i kernel);
QImage *F_close(QImage *image, U_Kernel_i kernel);

// 细化、粗化
QImage *F_complement(QImage *image);
QImage *F_union(QImage *image1, QImage *image2);
QImage *F_intersection(QImage *image1, QImage *image2);
QImage *F_hitAndMiss(QImage *image, U_Kernel_i kernel);

QImage *F_thinning(QImage *image, U_Kernel_i kernel);
QImage *F_thickening(QImage *image, U_Kernel_i kernel);

// 距离变换
QImage *F_contrastStretch(QImage *image);
QImage *F_distance(QImage *image);

// 骨架、骨架重构
QImage *F_skeletonize(QImage *image);
QImage *F_skeletonReconstruct(QImage *image);

// 二值形态学重构
QImage *F_reconstruct(QImage *marker, QImage *mask);

/***************************************************************
 * 9. 灰度数学形态学
****************************************************************/
// 分水岭算法
QImage *F_watershed(QImage *image);

/***************************************************************
 * 10. 霍夫变换
****************************************************************/
QImage *F_hough_line(QImage *image, int hough_limit);
QImage *F_hough_circle(QImage *image, int radius, int hough_limit);

#endif // PROCESSOR_H
