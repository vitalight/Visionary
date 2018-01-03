#if 0
#include "stdafx.h"
#include "watershed.h"

#include <map>
#include <set>
#include <list>
#include "math.h"
using namespace std;

namespace watershed
{

//每次注水深度
#define STEP_ASH 10
//用来标明尚未被淹没的像素
#define REGION_NO_DEFAULT -1
//用来限定最小区域的大小
#define MIN_REGIONPIXEL_COUNT 1
//容差,用来控制分割的精确度
#define TOLERANCE 28

//宏定义,为方便容器的遍历
#define FOR_EACH(pContainer, it)            \
    for ((it) = (pContainer)->begin(); (it) != (pContainer)->end(); (it)++)

//类型声明
typedef unsigned char BYTE;
typedef BYTE *ByteImage;
typedef long RegionNo;

//描述算法中单个像素
typedef struct _WatershedPixel
{
    int nX;              //像素坐标
    int nY;
    BYTE btAsh;          //像素灰度
    bool bDrown;         //是否已经被淹没
    RegionNo lRegionNo;  //所属区域的编号
} WatershedPixel;

//描述算法中被分割出的一个区域, 即集水盆
typedef struct _WatershedRegion
{
    RegionNo regionNo;       //区域编号
    BYTE btLowest;           //集水盆最深处的灰度
    WatershedPixel *pPixel;  //区域中作为种子的任意像素
    long lPixelCount;        //区域的大小
    long lTotalAsh;
}WatershedRegion;

//描述算法中的图像
typedef struct _WatershedImage
{
    BYTE bTolerance;         //容差
    int nWidth;              //图像尺寸
    int nHeight;
    WatershedPixel *pixels;  //像素集
    map<RegionNo, WatershedRegion *> regionMap;  //用于记录区域信息
}WatershedImage;

/****************************************************************************
* 通过坐标获得像素
* 参数：pWatershedImage 图像
*       nX, nY 像素坐标
****************************************************************************/
inline WatershedPixel *GetPixelXY(
    WatershedImage *pWatershedImage, 
    int nX, int nY
)
{
    long lOffset = nY * pWatershedImage->nWidth + nX;
    return &(pWatershedImage->pixels[lOffset]);
}

/****************************************************************************
* 生成一个唯一的区域编号
****************************************************************************/
inline RegionNo CreateRegionNo()
{
    static RegionNo lRegionNo = 0;
    lRegionNo += 1;
    return lRegionNo;
}

/****************************************************************************
* 检查坐标的合法性
* 参数：pWatershedImage 图像
*       nX, nY 像素坐标
****************************************************************************/
inline bool CheckXY(WatershedImage *pWatershedImage, int nX, int nY)
{
    return (nX >= 0 && nX < pWatershedImage->nWidth && 
        nY >= 0 && nY < pWatershedImage->nHeight);
}

/****************************************************************************
* 初始化图像
* 参数：pWatershedImage 分水岭图像
*       imageIn 输入的点阵图像
*       w, h 图像尺寸
*       btTolerance 容差
****************************************************************************/
bool InitWatershedImage(
    WatershedImage *pWatershedImage, 
    ByteImage imageIn, 
    unsigned int w, unsigned int h,
    BYTE btTolerance
)
{
    bool bRetn = false;
    long lIndex, lPixelOffset;
    int nTotalAsh;
    int nAsh;
    WatershedPixel *pPixels = NULL;
    WatershedPixel *pCurrentPixel;

    //创建像素集
    pPixels = new WatershedPixel[w * h];
    if (pPixels == NULL)
        goto Exit0;

    //保存参数
    pWatershedImage->nHeight = h;
    pWatershedImage->nWidth = w;
    pWatershedImage->pixels = pPixels; 
    pWatershedImage->bTolerance = btTolerance;
    pWatershedImage->regionMap.clear();

    //初始化分水岭图像
    for (lIndex = 0; lIndex < (long)(w * h); lIndex++)
    {
        lPixelOffset = lIndex * 4;
        nTotalAsh = imageIn[lPixelOffset] 
        + imageIn[lPixelOffset + 1]
        + imageIn[lPixelOffset + 2];
        pCurrentPixel = &(pPixels[lIndex]);
        pCurrentPixel->nX = lIndex % w;
        pCurrentPixel->nY = lIndex / w;

        //计算平均灰度，并格式化为STEP_ASH的整数倍
        nAsh = nTotalAsh / 3;
        nAsh = nAsh/ STEP_ASH * STEP_ASH;

        pCurrentPixel->btAsh = nAsh;
        pCurrentPixel->bDrown = false;
        pCurrentPixel->lRegionNo = REGION_NO_DEFAULT;
    }

    bRetn = true;
Exit0:
    return bRetn;
}

/****************************************************************************
* 释放图像
* 参数：pWatershedImage 分水岭图像
****************************************************************************/
bool FinalizeWatershedImage(
    WatershedImage *pWatershedImage
)
{
    bool bRetn = false;
    map<RegionNo, WatershedRegion *>::iterator itPair;
    WatershedRegion *pRegion;

    if (NULL == pWatershedImage)
        goto Exit0;

    FOR_EACH(&(pWatershedImage->regionMap), itPair)
    {
        pRegion = (*itPair).second;
        delete pRegion;
    }

    pWatershedImage->regionMap.clear();
    delete[] pWatershedImage->pixels;
    bRetn = true;
Exit0:
    return bRetn;
}

/****************************************************************************
* 将像素集合并到指定区域
* 参数：pPixelList 像素集
*       pRegion    目标区域
****************************************************************************/
bool AddPixelsToRegion(
    list<WatershedPixel *> *pPixelList, 
    WatershedRegion *pRegion
)
{
    list<WatershedPixel *>::iterator itPix;
    WatershedPixel *pPixel = NULL;
    RegionNo regionNo;
    long lTotalAsh;

    regionNo = pRegion->regionNo;
    lTotalAsh = pRegion->lTotalAsh;

    FOR_EACH(pPixelList, itPix)
    {
        pPixel = *itPix;
        pPixel->lRegionNo = regionNo;
        lTotalAsh += pPixel->btAsh;
    }
    pRegion->lPixelCount += (long)pPixelList->size();
    pRegion->lTotalAsh = lTotalAsh;

    return true;
}


/****************************************************************************
* 用种子算法搜索相同灰度的未淹没像素
* 参数：pSeedPixel    种子像素
*       pNeighbors    搜索过程中发现的相邻区域
*       pPixelsFound  搜索到的像素
*       *pTmpRegionNo 为搜索到的像素临时创建的区域
****************************************************************************/
bool SeedSearchPixel(
    WatershedImage *pWatershedImage,
    WatershedPixel *pSeedPixel,
    set<RegionNo> *pNeighbors,
    list<WatershedPixel *> *pPixelsFound,
    RegionNo *pTmpRegionNo
)
{
    //方向表
    static int dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    static int dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};

    //种子算法需要的临时队列
    static list<WatershedPixel *> list1, list2;

    RegionNo newRegionNo;
    list<WatershedPixel *>::iterator itPix;
    list<WatershedPixel *> *pCurrentList, *pNewList, *pSwapList;
    WatershedPixel *pNewPixel, *pPixel;
    WatershedRegion *pNewRegion = NULL;
    BYTE btAsh;

    int nIndex, nNx, nNy;
    long lPixelCount = 0;
    long lNeighborCount = 0;

    pCurrentList = &list1;
    pNewList = &list2;

    btAsh = pSeedPixel->btAsh;
    newRegionNo = CreateRegionNo();
    *pTmpRegionNo = newRegionNo;
    pCurrentList->clear();
    pCurrentList->push_back(pSeedPixel);
    pPixelsFound->push_back(pSeedPixel);
    pSeedPixel->lRegionNo = newRegionNo;
    pSeedPixel->bDrown = true;

    while (pCurrentList->size() > 0)
    {
        pNewList->clear();
        FOR_EACH(pCurrentList, itPix)
        {
            pPixel = *itPix;
            for (nIndex = 0; nIndex < 8; nIndex++)
            {
                nNx = pPixel->nX + dx[nIndex];
                nNy = pPixel->nY + dy[nIndex];
                if (!CheckXY(pWatershedImage, nNx, nNy))
                    continue;

                pNewPixel = GetPixelXY(pWatershedImage, nNx, nNy);
                //如果是本次搜索已经发现的像素则跳过
                if (pNewPixel->lRegionNo == newRegionNo)
                    continue;

                //如果是已经被淹没的像素,记录该像素所属区域编号
                if (pNewPixel->bDrown)
                {
                    pNeighbors->insert(pNewPixel->lRegionNo);
                    continue;
                }

                if (pNewPixel->btAsh != btAsh)
                    continue;

                //对于新发现的像素,标记其区域编号,并将其淹没
                pNewPixel->bDrown = true;
                pNewPixel->lRegionNo = newRegionNo;
                pNewList->push_back(pNewPixel);
                pPixelsFound->push_back(pNewPixel);
            }
        }
        pSwapList = pCurrentList;
        pCurrentList = pNewList;
        pNewList = pSwapList;
    }
    return true;
}

/****************************************************************************
* 用种子算法搜索指定区域的像素
* 参数：pRegion         要搜索的区域
*       pPixelsFound    搜索到的像素
****************************************************************************/
bool GetRegionPixels(
    WatershedImage *pWatershedImage, 
    WatershedRegion *pRegion, 
    list<WatershedPixel *> *pPixelsFound
)
{
    //bool bRetn = false;
    static int dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    static int dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    static list<WatershedPixel *> list1, list2;
    static set<WatershedPixel *> pixelsDone;

    list<WatershedPixel *>::iterator itPix;
    list<WatershedPixel *> *pCurrentList, *pNewList, *pSwapList;
    set<WatershedPixel *>::_Pairib pairIb;
    WatershedPixel *pNewPixel, *pPixel;
    WatershedRegion *pNewRegion = NULL;
    RegionNo regionNo;
    int nIndex, nNx, nNy;

    pCurrentList = &list1;
    pNewList = &list2;
    regionNo = pRegion->regionNo;

    pCurrentList->clear();
    pixelsDone.clear();
    pCurrentList->push_back(pRegion->pPixel);
    pPixelsFound->push_back(pRegion->pPixel);
    pixelsDone.insert(pRegion->pPixel);

    while (pCurrentList->size() > 0)
    {
        pNewList->clear();
        FOR_EACH(pCurrentList, itPix)
        {
            pPixel = *itPix;
            for (nIndex = 0; nIndex < 8; nIndex++)
            {
                nNx = pPixel->nX + dx[nIndex];
                nNy = pPixel->nY + dy[nIndex];
                if (!CheckXY(pWatershedImage, nNx, nNy))
                    continue;

                pNewPixel = GetPixelXY(pWatershedImage, nNx, nNy);
                if (pNewPixel->lRegionNo != regionNo)
                    continue;

                pairIb = pixelsDone.insert(pNewPixel);
                if (!pairIb.second)
                    continue;

                pNewList->push_back(pNewPixel);
                pPixelsFound->push_back(pNewPixel);
            }
        }
        pSwapList = pCurrentList;
        pCurrentList = pNewList;
        pNewList = pSwapList;
    }

    //bRetn = true;
    return true;
}

/****************************************************************************
* 合并两个不同区域
* 参数：pDestRegion   合并到的目标区域
*       pSrcRegion    被合并的区域
****************************************************************************/
bool CombineRegion(
    WatershedImage *pWatershedImage, 
    WatershedRegion *pDestRegion, 
    WatershedRegion *pSrcRegion
)
{
    static list<WatershedPixel *> pixelList;

    bool bRetn = false;
    bool b;
    list<WatershedPixel *>::iterator itPix;

    WatershedPixel *pPixel;
    RegionNo destRegionNo;
    BYTE btLowest = 255;

    //被合并的两个区域不能相同
    if (pDestRegion->regionNo == pSrcRegion->regionNo)
        goto Exit0;

    pixelList.clear();
    destRegionNo = pDestRegion->regionNo;

    //设定被合并后区域的深度
    if (pSrcRegion->btLowest < pDestRegion->btLowest)
    {
        btLowest = pSrcRegion->btLowest;
    }
    else
    {
        btLowest = pDestRegion->btLowest;
    }

    // 搜索被合并区域的所有像素
    b = GetRegionPixels(pWatershedImage, pSrcRegion, &pixelList);
    if (!b)
        goto Exit0;

    // 更改被合并区域像素的区域编号
    FOR_EACH(&pixelList, itPix)
    {
        pPixel = (*itPix);
        pPixel->lRegionNo = destRegionNo;
    }

    pDestRegion->btLowest = btLowest;
    pDestRegion->lPixelCount += pSrcRegion->lPixelCount;
    pDestRegion->lTotalAsh += pSrcRegion->lTotalAsh;

    // 从图像中删除被合并的区域
    pWatershedImage->regionMap.erase(pSrcRegion->regionNo);
    delete pSrcRegion;

    bRetn = true;
Exit0:
    return bRetn;
}

bool CreateNewRegion(
    WatershedImage *pWatershedImage,
    list<WatershedPixel *> *pNewPixels,
    RegionNo newRegionNo,
    BYTE btLowest
)
{
    bool bRetn = false;
    WatershedRegion *pNewRegion;
    WatershedPixel *pPixel;
    list<WatershedPixel *>::iterator itPix;
    long lTotalAsh = 0;

    FOR_EACH(pNewPixels, itPix)
    {
        pPixel = *itPix;
        lTotalAsh += pPixel->btAsh;
    }

    pNewRegion = new WatershedRegion();
    if (NULL == pNewRegion)
        goto Exit0;
    pNewRegion->btLowest = btLowest;
    pNewRegion->lPixelCount = (long)pNewPixels->size();
    pNewRegion->regionNo = newRegionNo;
    pNewRegion->pPixel = *(pNewPixels->begin());
    pNewRegion->lTotalAsh = lTotalAsh;
    pWatershedImage->regionMap.insert(
        make_pair(newRegionNo, pNewRegion));

    bRetn = true;
Exit0:
    return bRetn;
}

/****************************************************************************
* 用于将新淹没的像素合并到已有区域，或创建新的区域， 
* 未被合并的区域之间可理解为被筑了堤坝
* 参数：btAsh         当前淹没的深度
*       pNewPixels    新淹没的像素
*       pNeighbors    新淹没像素相邻的区域
*       newRegionNo   新淹没像素所属的临时区域编号
****************************************************************************/
bool CombineAndDam(
    WatershedImage *pWatershedImage,
    BYTE btAsh,
    list<WatershedPixel *> *pNewPixels,
    set<RegionNo> *pNeighbors,
    RegionNo newRegionNo
)
{
    bool bRetn = false;
    set<WatershedRegion *>::iterator itRegion;
    set<RegionNo>::iterator itRegionNo;
    map<RegionNo, WatershedRegion *>::iterator itPair;
    WatershedRegion *pRegion;
    WatershedRegion *pNewRegion = NULL;
    WatershedRegion *pDeepRegionToCombine = NULL;
    RegionNo regionNo;
    long lDeepRegionCount = 0;
    bool b, bCombined = false;
    BYTE btAverage = 0;

    if (pNewPixels->size() == 0)
        goto Exit0;

    // 依次检查新淹没区域的临近区域
    FOR_EACH(pNeighbors, itRegionNo)
    {
        regionNo = (*itRegionNo);
        itPair = pWatershedImage->regionMap.find(regionNo);
        if (itPair == pWatershedImage->regionMap.end())
            goto Exit0;
        pRegion = (*itPair).second;
        if (btAsh < pRegion->btLowest)
            goto Exit0;

        // 如果临近的区域为浅水池或小水池，则与其合并
        if ((btAsh - pRegion->btLowest < pWatershedImage->bTolerance) ||
            (pRegion->lPixelCount < MIN_REGIONPIXEL_COUNT))
        {
            if (!bCombined)
            {
                b = AddPixelsToRegion(pNewPixels, pRegion);
                if (!b)
                    goto Exit0;
                bCombined = true;
                pNewRegion = pRegion;
            }
            else
            {
                b = CombineRegion(pWatershedImage, pNewRegion, pRegion);
                if (!b)
                    goto Exit0;
            }
            continue;
        }

        //如果临近的区域为深水池,则选择平均灰度与当前最接近的一个进行合并
        if (NULL == pDeepRegionToCombine)
        {
            pDeepRegionToCombine = pRegion;
            btAverage = (BYTE)(pRegion->lTotalAsh / pRegion->lPixelCount);
            lDeepRegionCount += 1;
        }
        else
        {
            if ((BYTE)(pRegion->lTotalAsh / pRegion->lPixelCount) > btAverage)
            {
                pDeepRegionToCombine = pRegion;
            }
            lDeepRegionCount += 1;
        }
    }

    // 如果发现了最小的深水池,
    // 则把新淹没区域连同相邻的浅水池一同与其合并
    if (NULL != pDeepRegionToCombine)
    {
        if (!bCombined)
        {
            b = AddPixelsToRegion(pNewPixels, pDeepRegionToCombine);
            if (!b)
                goto Exit0;
            bCombined = true;
        }
        else
        {
            b = CombineRegion(pWatershedImage, pDeepRegionToCombine, pNewRegion);
            if (!b)
                goto Exit0;
        }
    }
    // 否则, 如果新淹没区域没有与任何区域相邻
    // 为新淹没区域创建新的区域
    else
    {
        if (!bCombined)
        {
            b = CreateNewRegion(pWatershedImage, pNewPixels, newRegionNo, btAsh);
            if (!b)
                goto Exit0;
        }
    }

    bRetn = true;
Exit0:
    return bRetn;
}

/****************************************************************************
* 淹没到指定深度
* 参数：btAsh        淹没的深度
****************************************************************************/
bool Flood(WatershedImage *pWatershedImage, BYTE btAsh)
{
    static set<RegionNo> neighbor;
    static list<WatershedPixel *> pixelsFound;

    bool bRetn = false;
    bool b;
    long lIndex, lCount;
    WatershedPixel *pPixel;
    RegionNo tmpRegionNo;

    lCount = pWatershedImage->nHeight * pWatershedImage->nWidth;
    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        pPixel = &pWatershedImage->pixels[lIndex];
        if (pPixel->btAsh != btAsh)
            continue;
        if (pPixel->bDrown)
            continue;
        neighbor.clear();
        pixelsFound.clear();
        // 寻找新淹没的单个区域
        b = SeedSearchPixel(
            pWatershedImage, pPixel, 
            &neighbor, &pixelsFound, &tmpRegionNo);

        if (!b)
            goto Exit0;

        // 决定将新淹没的区域与临近区域合并活筑坝
        b = CombineAndDam(
            pWatershedImage, btAsh, 
            &pixelsFound, &neighbor, tmpRegionNo);
        if (!b)
            goto Exit0;
    }
    bRetn = true;
Exit0:
    return bRetn;
}

/****************************************************************************
* 将分割得到的区域用不同的颜色标明在输出图像上
* 参数：imageOut        输出的图像
****************************************************************************/
bool OutputToByteImage(WatershedImage *pWatershedImage, ByteImage imageOut)
{
    static set<RegionNo> neighbor;
    bool bRetn = false;
    map<RegionNo, WatershedRegion *>::iterator itPair;
    long lIndex, lCount;
    WatershedPixel *pPixel;
    WatershedRegion *pRegion = NULL;
    BYTE btAsh;

    lCount = pWatershedImage->nHeight * pWatershedImage->nWidth;
    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        pPixel = &pWatershedImage->pixels[lIndex];
        //根据区域编号产生随机的颜色
        itPair = pWatershedImage->regionMap.find(pPixel->lRegionNo);
        if (itPair == pWatershedImage->regionMap.end())
            goto Exit0;
        pRegion = (*itPair).second;
        btAsh = (BYTE)(pRegion->lTotalAsh / pRegion->lPixelCount);

        imageOut[lIndex * 4  + 0] = btAsh;
        imageOut[lIndex * 4  + 1] = btAsh;
        imageOut[lIndex * 4  + 2] = btAsh;
        imageOut[lIndex * 4  + 3] = 255;
    }

    bRetn = true;
Exit0:
    return bRetn;
}


/********************************************************************************
* 分水岭分割算法
* 参数：imageIn     输入的图像
*       imageOut    输出的图像
*       w, h        图像尺寸
********************************************************************************/
void __stdcall Watershed(
    BYTE *imageIn, BYTE * imageOut, 
    unsigned int w, unsigned int h
                         )
{
    WatershedImage watershedImage;
    bool bRetnSuccess;
    int nAsh;
    BYTE btAsh;

    bRetnSuccess = InitWatershedImage(&watershedImage, imageIn, w, h, TOLERANCE);
    if (!bRetnSuccess)
        goto Exit0;

    for (nAsh = 0; nAsh < 255; nAsh += STEP_ASH)
    {
        btAsh = (BYTE)(nAsh & 0xFF);
        bRetnSuccess = Flood(&watershedImage, btAsh);
        if (!bRetnSuccess)
            goto Exit0;
    }
    bRetnSuccess = OutputToByteImage(&watershedImage, imageOut);
    if (!bRetnSuccess)
        goto Exit0;

    bRetnSuccess = FinalizeWatershedImage(&watershedImage);
    if (!bRetnSuccess)
        goto Exit0;

Exit0:
    return;
}

}
#endif
