#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <vector>
#include <math.h>

typedef std::vector<std::vector<double>> U_Kernel_d;
typedef std::vector<std::vector<int>> U_Kernel_i;

class U_Point
{
public:
    int x;
    int y;

    U_Point(int _x, int _y)
        :x(_x), y(_y)
    {
    }
};

std::vector<std::vector<double>> U_getFlatKernel_d(int size, double val = 1);
std::vector<std::vector<int>> U_getFlatKernel_i(int size, int val = 1);
std::vector<std::vector<double>> U_getGaussianKernel(int size, double sigma);

int U_getKernelSum(U_Kernel_i kernel);
double U_getKernelSum(U_Kernel_d kernel);
int U_round(double f);
bool U_in(int low, int value, int high);
bool U_legal(int width, int height, double x, double y);
void U_colorBound(int &r, int &g, int &b);

#endif // UTIL_H
