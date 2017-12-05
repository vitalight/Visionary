#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <Qimage>

double F_responseTime();

QImage *F_decolor(QImage *image);
QImage *F_binarization(QImage *image, int threshold);
QImage *F_convolution(QImage *image, int kernel[], int kernelSize, int kernelSum);
QImage *F_blur(QImage *image);
QImage *F_sharpen(QImage *image);
QImage *F_dilation(QImage *image);
QImage *F_erosion(QImage *image);

#endif // PROCESSOR_H
