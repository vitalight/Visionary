#include "util.h"
using namespace std;

static const double PI = 4.0*atan(1.0);

vector<vector<double>> U_getGaussianKernel(int size, double sigma)
{
    vector<vector<double>> kernel;
    int center = size/2;
    double sum = 0, val;
    for (int i=0; i<size; i++)
    {
        vector<double> line;
        for (int j=0; j<size; j++)
        {
            val = (1/(2*PI*sigma*sigma))*exp(-((i-center)*(i-center)+(j-center)*(j-center))/(2*sigma*sigma));
            line.push_back(val);
            sum += val;
        }
        kernel.push_back(line);
    }

    for (int i=0; i<size; i++)
    {
        for (int j=0; j<size; j++)
        {
            kernel[i][j]/=sum;
        }
    }
    return kernel;
}

vector<vector<double>> U_getFlatKernel_d(int size, double val)
{
    vector<vector<double>> kernel;

    for (int i=0; i<size; i++)
    {
        vector<double> line(size, val);
        kernel.push_back(line);
    }

    return kernel;
}

vector<vector<int>> U_getFlatKernel_i(int size, int val)
{
    vector<vector<int>> kernel;

    for (int i=0; i<size; i++)
    {
        vector<int> line(size, val);
        kernel.push_back(line);
    }

    return kernel;
}

int U_round(double f)
{
    return (int)(f+0.5);
}

bool U_in(int low, int value, int high)
{
    return value>=low && value<high;
}

bool U_legal(int width, int height, int x, int y)
{
    return x>=0 && x<width && y>=0 && y<height;
}

void U_colorBound(int &r, int &g, int &b)
{
    if (r>255)
        r = 255;
    else if (r<0)
        r = 0;

    if (g>255)
        g = 255;
    else if (g<0)
        g = 0;

    if (b>255)
        b = 255;
    else if (b<0)
        b = 0;
}
