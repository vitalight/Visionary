#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <vector>
#include <math.h>

class U_Point
{
public:
    int x;
    int y;

    U_Point(int x_, int y_)
        :x(x_), y(y_)
    {
    }
};

std::vector<std::vector<double>> U_getFlatKernel_d(int size, double val = 1);
std::vector<std::vector<int>> U_getFlatKernel_i(int size, int val = 1);
std::vector<std::vector<double>> U_getGaussianKernel(int size, double sigma);

int U_round(double f);
bool U_in(int low, int value, int high);
bool U_legal(int width, int height, int x, int y);
void U_colorBound(int &r, int &g, int &b);

#endif // UTIL_H
