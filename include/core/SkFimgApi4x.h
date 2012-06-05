/*
 * Copyright 2009, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SkFimgApi_DEFINED
#define SkFimgApi_DEFINED

#if defined(FIMG2D_ENABLED)

#include "SkColorPriv.h"
#include "SkBitmap.h"
#include "SkMallocPixelRef.h"
#include "SkFlattenable.h"
#include "SkUtils.h"
#include "SkXfermode.h"
#include "SkMatrix.h"
#include "SkBitmap.h"
#include "SkMask.h"

#include "FimgApi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include <linux/android_pmem.h>

//---------------------------------------------------------------------------//

#define TRUE 1
#define FALSE 0
#define FIMGAPI_G2D_BLOKING             TRUE
#define FIMGAPI_COPROMISE_USE           TRUE
#define FIMGAPI_HYBRID_USE              FALSE

#if defined(SWP1_CUSTOMSIMD_ENABLE)
#define FIMGAPI_DMC_SIMD_OPT_USE        TRUE
#else
#define FIMGAPI_DMC_SIMD_OPT_USE        FALSE
#endif

#define FIMGAPI_FINISHED        (0x1<<0)
#define FIMGAPI_HYBRID          (0x1<<1)
/*       G2D Compromise values          */
#define COMP_VALUE_FILTER_SCALE 48*80
#define COPT_NANS       0
#define COPT_ANS        1
#define COPT_NAS        2
#define COPT_AS         3
#undef FIMGAPI_DEBUG_MESSAGE

struct Fimg {
    int             srcX;
    int             srcY;
    unsigned int    srcW;
    unsigned int    srcH;
    unsigned int    srcFWStride; // this is not w, just stride (w * bpp)
    unsigned int    srcFH;
    unsigned int    srcBPP;
    int             srcColorFormat;
    unsigned char  *srcAddr;

    int             isSolidFill;

    int             dstX;
    int             dstY;
    unsigned int    dstW;
    unsigned int    dstH;
    unsigned int    dstFWStride; // this is not w, just stride (w * bpp)
    unsigned int    dstFH;
    unsigned int    dstBPP;
    int             dstColorFormat;
    unsigned char  *dstAddr;

    int             clipT;
    int             clipB;
    int             clipL;
    int             clipR;

    int             mskX;
    int             mskY;
    unsigned int    mskW;
    unsigned int    mskH;
    unsigned int    mskFWStride; // this is not w, just stride (w * bpp)
    unsigned int    mskFH;
    unsigned int    mskBPP;
    int             mskColorFormat;
    unsigned char  *mskAddr;

    unsigned long   fillcolor;
    int             rotate;
    int             alpha;
    int             xfermode;
    int             isDither;
    int             isFilter;
    int             colorFilter;
    int             canusehybrid;
    int             matrixType;
    SkScalar        matrixSx;
    SkScalar        matrixSy;

    Fimg() {
        int             srcX = 0;
        int             srcY = 0;
        unsigned int    srcW = 0;
        unsigned int    srcH = 0;
        unsigned int    srcFWStride = 0;
        unsigned int    srcFH = 0;
        unsigned int    srcBPP = 0;
        int             srcColorFormat = 0;
        unsigned char  *srcAddr = (unsigned char *)NULL;

        int             isSolidFill = 0;

        int             dstX = 0;
        int             dstY = 0;
        unsigned int    dstW = 0;
        unsigned int    dstH = 0;
        unsigned int    dstFWStride = 0;
        unsigned int    dstFH = 0;
        unsigned int    dstBPP = 0;
        int             dstColorFormat = 0;
        unsigned char  *dstAddr = (unsigned char *)NULL;

        int             clipT = 0;
        int             clipB = 0;
        int             clipL = 0;
        int             clipR = 0;

        int             mskX = 0;
        int             mskY = 0;
        unsigned int    mskW = 0;
        unsigned int    mskH = 0;
        unsigned int    mskFWStride = 0;
        unsigned int    mskFH = 0;
        unsigned int    mskBPP = 0;
        int             mskColorFormat = 0;
        unsigned char  *mskAddr = (unsigned char *)NULL;

        unsigned long   fillcolor = 0;
        int             rotate = 0;
        int             alpha = 0;
        int             xfermode = 0;
        int             isDither = 0;
        int             isFilter = 0;
        int             colorFilter = 0;
        int             canusehybrid = 0;
        int             matrixType = 0;
        SkScalar        matrixSx = 0;
        SkScalar        matrixSy =  0;
    }
};

int FimgApiCheckPossibleHybrid(Fimg *fimg);
bool FimgApiCheckPossible(Fimg *fimg);
bool FimgApiCheckPossiblePorterDuff(Fimg *fimg);
bool FimgApiIsDstMode(Fimg *fimg);
bool FimgApiCompromise(Fimg *fimg);
int FimgApiStretch(Fimg *fimg, const char *func_name);
int FimgApiSync(const char *func_name);
#endif //defined(FIMG2D_ENABLED)

#ifdef CHECK_TIME_FOR_FIMGAPI

extern "C" {
    #include <stdio.h>
    #include <sys/time.h>
    #include <time.h>
    #include <unistd.h>
}

class MyAutoTimeManager
{
private:
    const char     *mInfo;

    int             mSrcColorFormat;
    int             mSrcWidth;
    int             mSrcHeight;

    int             mDstColorFormat;
    int             mDstWidth;
    int             mDstHeight;
    int             mAlpha;

    char           *nameA;
    long long       timeA;
    char           *nameB;
    long long       timeB;

public:
    MyAutoTimeManager(const char *info,
                      int srcColorFormat,
                      int srcWidth,
                      int srcHeight,
                      int dstColorFormat,
                      int dstWidth,
                      int dstHeight,
                      int alpha)
               : mInfo(info),
                 mSrcColorFormat(srcColorFormat),
                 mSrcWidth (srcWidth),
                 mSrcHeight(srcHeight),
                 mDstColorFormat(dstColorFormat),
                 mDstWidth (dstWidth),
                 mDstHeight(dstHeight),
                 mAlpha(alpha)
    {
        nameA = NULL;
        timeA = 0;
        nameB = NULL;
        timeB = 0;
    }

    ~MyAutoTimeManager()
    {
        if (nameA && nameB
            && FimgApiCheckPossible(mSrcColorFormat, mSrcWidth, mSrcHeight,
                                    mDstColorFormat, mDstWidth, mDstHeight) == true) {
            long long gapTime;
            float     gapRate;
            long long fasterTime;
            long long slowerTime;
            char     *fasterName;
            char     *slowerName;

            if (timeA <= timeB) {
                fasterName = nameA;
                fasterTime = timeA;
                slowerName = nameB;
                slowerTime = timeB;
            } else {
                fasterName = nameB;
                fasterTime = timeB;
                slowerName = nameA;
                slowerTime = timeA;
            }

            gapTime = slowerTime - fasterTime;
            gapRate = ((float)slowerTime / (float)fasterTime) * 100.f - 100.0f;

            {
                SkDebugf("%s::%s (%5lld) faster than %s (%5lld) (%5lld) microsec / (%6.1f ) %% : [%3d %3d (%2d)] -> [%3d %3d (%2d)] \[sizeConv : %c] [colorConv : %c] [alpha : %3d] \n",
                          mInfo,
                          fasterName,
                          fasterTime,
                          slowerName,
                          slowerTime,
                          gapTime,
                          gapRate,
                          mSrcWidth,
                          mSrcHeight,
                          mSrcColorFormat,
                          mDstWidth,
                          mDstHeight,
                          mDstColorFormat,
                          (mSrcWidth != mDstWidth || mSrcHeight != mDstHeight) ? 'O' : 'X',
                          (mSrcColorFormat != mDstColorFormat) ? 'O' : 'X',
                          mAlpha);
            }
        }
    }

    void SetGap(char *name, long long gap)
    {
        if (nameA == NULL) {
            nameA = name;
            timeA = gap;
        } else {
            nameB = name;
            timeB = gap;
        }
    }
};

//---------------------------------------------------------------------------//

class MyAutoTime
{
private:
    struct timeval     mStartWhen;
    const char        *mMyName;
    MyAutoTimeManager *mAutoTimeManager;

public:
    MyAutoTime(const char *myName,
               MyAutoTimeManager *autoTimeManager)
               : mMyName(myName),
                 mAutoTimeManager(autoTimeManager)
    {
        gettimeofday(&mStartWhen, NULL);
    }

    ~MyAutoTime()
    {
        struct timeval endWhen;
        long long      timeGap;

        gettimeofday(&endWhen, NULL);

        long long start = ((long long) mStartWhen.tv_sec) * 1000000LL + ((long long) mStartWhen.tv_usec);
        long long stop = ((long long) endWhen.tv_sec) * 1000000LL + ((long long) endWhen.tv_usec);

        timeGap = stop - start;
        mAutoTimeManager->SetGap((char*)mMyName, timeGap);
    }
};

#endif
int FimgARGB32_BW(struct Fimg &fimg,  const SkBitmap &device,  const SkMask &mask,  const SkIRect &clip,
                  const SkPMColor &color,  const unsigned int alpha);
int FimgARGB32_Rect(struct Fimg &fimg,  uint32_t *device,  int x,  int y,  int width,  int height,
                    size_t rowbyte,  uint32_t color);

uint32_t toARGB32(uint32_t color);
#endif //SkFimgApi_DEFINED
