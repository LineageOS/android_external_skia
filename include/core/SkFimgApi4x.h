/*
 * Copyright 2012, Samsung Electronics Co. LTD
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

//---------------------------------------------------------------------------//

#define FIMGAPI_COMPROMISE_USE true

#define FIMGAPI_FINISHED       (0x1<<0)
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
    unsigned int    alpha;
    int             xfermode;
    int             isDither;
    int             isFilter;
    int             colorFilter;
    int             matrixType;
    SkScalar        matrixSx;
    SkScalar        matrixSy;
};

bool FimgApiCheckPossible(Fimg *fimg);
bool FimgApiIsDstMode(Fimg *fimg);
bool FimgApiCompromise(Fimg *fimg);
int FimgApiStretch(Fimg *fimg, const char *func_name);
int FimgARGB32_Rect(struct Fimg &fimg,  uint32_t *device,  int x,  int y,  int width,  int height,
                    size_t rowbyte,  uint32_t color);
uint32_t toARGB32(uint32_t color);
#endif //SkFimgApi_DEFINED
