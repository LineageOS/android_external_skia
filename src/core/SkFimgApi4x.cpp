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
#if defined(FIMG2D_ENABLED)
#define LOG_TAG "SKIA"

#include <utils/Log.h>
#include <stdlib.h>
#include "SkFimgApi4x.h"

#define G2D_BLOCKING_USE 0

/* int comp_vlaue[dst][src][option]
 *     [dst]
 *     0 : kRGB_565_Config
 *     1 : kARGB_4444_Config
 *     2 : kARGB_8888_Config
 *     [src]
 *     0 : kRGB_565_Config
 *     1 : kARGB_4444_Config
 *     2 : kARGB_8888_Config
 *     [option]
 *         global_alpha / scale
 *     0 :      X       /   X
 *     1 :      O       /   X
 *     2 :      X       /   O
 *     3 :      O       /   O
 */
#if FIMGAPI_DMC_SIMD_OPT_USE
#if FIMGAPI_HYBRID_USE
int comp_value[3][3][4] = {
    {{360*600, 78*130, 168*280, 66*110}, {78*130, 54*90, 80*140, 66*110}, {168*280, 162*270, 102*170, 72*120}},
    {{54*90, 42*70, 54*90, 42*70}, {54*90, 48*80, 54*90, 48*80}, {66*110, 54*90, 66*110, 54*90}},
    {{84*140, 54*90, 84*130, 54*90}, {102*170, 78*130, 96*160, 78*130}, {120*200, 66*110, 96*160, 72*120}}
};
#else
int comp_value[3][3][4] = {
    {{480*800, 78*130, 168*280, 66*110}, {78*130, 54*90, 80*140, 66*110}, {204*340, 216*360, 102*170, 72*120}},
    {{54*90, 42*70, 54*90, 42*70}, {54*90, 48*80, 54*90, 48*80}, {66*110, 54*90, 66*110, 54*90}},
    {{84*140, 54*90, 84*130, 54*90}, {102*170, 78*130, 96*160, 78*130}, {180*300, 66*110, 96*160, 72*120}}
};
#endif
#else
#if FIMGAPI_HYBRID_USE
int comp_value[3][3][4] = {
    {{360*600, 78*130, 168*280, 66*110}, {78*130, 54*90, 80*140, 66*110}, {168*280, 162*270, 102*170, 72*120}},
    {{54*90, 42*70, 54*90, 42*70}, {54*90, 48*80, 54*90, 48*80}, {66*110, 54*90, 66*110, 54*90}},
    {{84*140, 54*90, 84*130, 54*90}, {102*170, 78*130, 96*160, 78*130}, {120*200, 66*110, 96*160, 72*120}}
};
#else
int comp_value[3][3][4] = {
    {{480*800, 78*130, 168*280, 66*110}, {78*130, 54*90, 80*140, 66*110}, {204*340, 216*360, 102*170, 72*120}},
    {{54*90, 42*70, 54*90, 42*70}, {54*90, 48*80, 54*90, 48*80}, {66*110, 54*90, 66*110, 54*90}},
    {{84*140, 54*90, 84*130, 54*90}, {102*170, 78*130, 96*160, 78*130}, {180*300, 66*110, 96*160, 72*120}}
};
#endif
#endif

enum color_format formatSkiaToDriver[] = {
    SRC_DST_FORMAT_END, //!< bitmap has not been configured
    CF_MSK_1BIT,
    CF_MSK_8BIT,
    SRC_DST_FORMAT_END, //!< kIndex8_Config is not supported by FIMG2D
    CF_RGB_565,
    CF_ARGB_4444,
    CF_ARGB_8888,
    SRC_DST_FORMAT_END, //!< kRLE_Index8_Config is not supported by FIMG2D
};

enum blit_op blendingSkiaToDriver[] = {
    BLIT_OP_CLR,
    BLIT_OP_SRC,
    BLIT_OP_DST,
    BLIT_OP_SRC_OVER,
    BLIT_OP_DST_OVER,
    BLIT_OP_SRC_IN,
    BLIT_OP_DST_IN,
    BLIT_OP_SRC_OUT,
    BLIT_OP_DST_OUT,
    BLIT_OP_SRC_ATOP,
    BLIT_OP_DST_ATOP,
    BLIT_OP_XOR,

    BLIT_OP_ADD, // kPlus_Mode
    BLIT_OP_MULTIPLY,

    BLIT_OP_SCREEN, // kScreen_Mode = kCoeffModesCnt
    BLIT_OP_NOT_SUPPORTED, // kOverlay_Mode
    BLIT_OP_DARKEN,
    BLIT_OP_LIGHTEN,

    BLIT_OP_NOT_SUPPORTED, //kColorDodge_Mode,
    BLIT_OP_NOT_SUPPORTED, //kColorBurn_Mode,
    BLIT_OP_NOT_SUPPORTED, //kHardLight_Mode,
    BLIT_OP_NOT_SUPPORTED, //kSoftLight_Mode,
    BLIT_OP_NOT_SUPPORTED, //kDifference_Mode,
    BLIT_OP_NOT_SUPPORTED, //kExclusion_Mode, kLastMode
};

enum scaling filterSkiaToDriver[] = {
    SCALING_NEAREST,
    SCALING_BILINEAR,
};

int FimgApiCheckPossibleHybrid(Fimg *fimg)
{
    if (!((fimg->srcW == fimg->dstW) && (fimg->srcH == fimg->dstH)))
        return 0;

    if (!((fimg->clipB - fimg->clipT) >= 40))
        return 0;

    if (fimg->canusehybrid == 0)
        return 0;

    if (fimg->srcColorFormat == SkBitmap::kARGB_8888_Config) {
        if (fimg->dstColorFormat == SkBitmap::kARGB_8888_Config) {
            if (fimg->alpha < G2D_ALPHA_VALUE_MAX)
                return 9;
            else
                return 7;
        } else if (fimg->dstColorFormat == SkBitmap::kRGB_565_Config) {
            if (fimg->alpha < G2D_ALPHA_VALUE_MAX)
                return 7;
            else
                return 7;
        }
    } else if (fimg->srcColorFormat == SkBitmap::kRGB_565_Config) {
        if (fimg->dstColorFormat == SkBitmap::kRGB_565_Config) {
            if (fimg->alpha < G2D_ALPHA_VALUE_MAX)
                return 9;
            else
                return 5;
        }
    }
        return 0;
}

bool FimgApiCheckPossible(Fimg *fimg)
{
    if (fimg->srcAddr != NULL) {
        switch (fimg->srcColorFormat) {
        case SkBitmap::kRGB_565_Config:
        case SkBitmap::kARGB_8888_Config:
        case SkBitmap::kARGB_4444_Config:
            break;
        default:
            return false;
        }
    }

    switch (fimg->dstColorFormat) {
    case SkBitmap::kRGB_565_Config:
        break;
    case SkBitmap::kARGB_8888_Config:
        break;
    case SkBitmap::kARGB_4444_Config:
        break;
    default:
        return false;
    }

    switch (fimg->xfermode) {
    case SkXfermode::kSrcOver_Mode:
        break;
    case SkXfermode::kClear_Mode:
        break;
    case SkXfermode::kSrc_Mode:
        break;
    case SkXfermode::kDst_Mode:
        break;
    case SkXfermode::kSrcIn_Mode:
        break;
    case SkXfermode::kDstIn_Mode:
        break;
    case SkXfermode::kDstOut_Mode:
        break;
    case SkXfermode::kSrcATop_Mode:
        break;
    case SkXfermode::kPlus_Mode:
        break;
    case SkXfermode::kMultiply_Mode:
        break;
    case SkXfermode::kScreen_Mode:
        break;
    case SkXfermode::kDarken_Mode:
        break;
    case SkXfermode::kLighten_Mode:
        break;
    case SkXfermode::kDstOver_Mode:
        break;
    case SkXfermode::kSrcOut_Mode:
        break;
    case SkXfermode::kDstATop_Mode:
        break;
    case SkXfermode::kXor_Mode:
        break;
    default:
        return false;
    }

    if (fimg->colorFilter != NULL)
        return false;

    if (fimg->matrixType & SkMatrix::kAffine_Mask)
        return false;

    if ((fimg->matrixSx < 0) || (fimg->matrixSy < 0))
        return false;

    if ((fimg->srcX + fimg->srcW) > 8000 || (fimg->srcY + fimg->srcH) > 8000)
        return false;

    if ((fimg->dstX + fimg->dstW) > 8000 || (fimg->dstY + fimg->dstH) > 8000)
        return false;

    if ((fimg->clipT < 0) || (fimg->clipB < 0) || (fimg->clipL < 0) || (fimg->clipR < 0)) {
        SkDebugf("Invalid clip value: TBLR = (%d, %d, %d, %d)",fimg->clipT, fimg->clipB, fimg->clipL, fimg->clipR);
        return false;
    }

    if ((fimg->clipT >= fimg->clipB) || (fimg->clipL >= fimg->clipR)) {
        SkDebugf("Invalid clip value: TBLR = (%d, %d, %d, %d)",fimg->clipT, fimg->clipB, fimg->clipL, fimg->clipR);
        return false;
    }

    return true;
}

bool FimgApiCheckPossiblePorterDuff(Fimg *fimg)
{
    if ((fimg->xfermode == SkXfermode::kMultiply_Mode
         || fimg->xfermode == SkXfermode::kScreen_Mode
         || fimg->xfermode == SkXfermode::kLighten_Mode
         || fimg->xfermode == SkXfermode::kDarken_Mode
         || fimg->xfermode == SkXfermode::kDstOver_Mode
         || fimg->xfermode == SkXfermode::kSrcOut_Mode
         || fimg->xfermode == SkXfermode::kDstATop_Mode
         || fimg->xfermode == SkXfermode::kXor_Mode)
        && fimg->alpha != 255)
        return false;

    return true;
}

bool FimgApiIsDstMode(Fimg *fimg)
{
    if (fimg->xfermode == SkXfermode::kDst_Mode)
        return true;
    else
        return false;
}

bool FimgApiCheckPossible_Clipping(Fimg *fimg)
{
    if (((fimg->clipR - fimg->clipL) <= 0) || ((fimg->clipB - fimg->clipT) <= 0))
        return false;
    return true;
}

bool FimgApiCompromise(Fimg *fimg)
{
    int comp_opt = 0;
    if ((fimg->srcW != fimg->dstW) || (fimg->srcH != fimg->dstH)) {
        if ((fimg->alpha != 256) && (fimg->alpha != 255))
            comp_opt = COPT_AS;
        else
            comp_opt = COPT_NAS;
    } else {
        if ((fimg->alpha != 256) && (fimg->alpha != 255))
            comp_opt = COPT_ANS;
        else
            comp_opt = COPT_NANS;
    }

    if ((fimg->isFilter == true) && ((comp_opt == COPT_AS) || (comp_opt == COPT_NAS)))
        if (((fimg->clipR - fimg->clipL) * (fimg->clipB - fimg->clipT)) >= COMP_VALUE_FILTER_SCALE)
            return true;

    if ((((fimg->clipR - fimg->clipL)*1.2) * ((fimg->clipB - fimg->clipT)*0.8)) < comp_value[fimg->dstColorFormat -4][fimg->srcColorFormat - 4][comp_opt])
        return false;

    return true;
}

int FimgApiStretch(Fimg *fimg, const char *func_name)
{
    int ret;
    static unsigned int seq_no = 100;

    struct fimg2d_blit cmd;
    struct fimg2d_scale scale;
    struct fimg2d_repeat repeat;
    struct fimg2d_bluscr bluScr;
    struct fimg2d_clip clip;
    struct fimg2d_image srcImage, dstImage, mskImage;
    struct fimg2d_rect srcRect, dstRect, mskRect;

    if (FimgApiCheckPossible(fimg) == false)
        return false;

    if (FimgApiCheckPossiblePorterDuff(fimg) == false)
        return false;

    if (FimgApiIsDstMode(fimg) == true)
        return FIMGAPI_FINISHED;

    if (fimg->clipL < fimg->dstX)
        fimg->clipL = fimg->dstX;
    if (fimg->clipT < fimg->dstY)
        fimg->clipT = fimg->dstY;
    if (fimg->clipR > (fimg->dstX + fimg->dstW))
        fimg->clipR = fimg->dstX + fimg->dstW;
    if (fimg->clipB > (fimg->dstY + fimg->dstH))
        fimg->clipB = fimg->dstY + fimg->dstH;

#if FIMGAPI_COPROMISE_USE
    if (FimgApiCompromise(fimg) == false)
        return false;
#endif
    enum rotation rotate;

    if (fimg->srcAddr != NULL && fimg->srcColorFormat != SkBitmap::kRGB_565_Config
        && fimg->srcColorFormat != SkBitmap::kARGB_4444_Config
        && fimg->srcColorFormat != SkBitmap::kARGB_8888_Config)
        return false;

    if (fimg->dstColorFormat != SkBitmap::kRGB_565_Config
        && fimg->dstColorFormat != SkBitmap::kARGB_4444_Config
        && fimg->dstColorFormat != SkBitmap::kARGB_8888_Config)
        return false;

    switch (fimg->rotate) {
    case 0:
        rotate = ORIGIN;
        break;
    case 90:
        rotate = ROT_90;
        break;
    case 180:
        rotate = ROT_180;
        break;
    case 270:
        rotate = ROT_270;
        break;
    default:
        return false;
    }

    cmd.op = blendingSkiaToDriver[fimg->xfermode];
    cmd.g_alpha = fimg->alpha;
    cmd.premult = PREMULTIPLIED;
    cmd.dither = fimg->isDither;
    cmd.rotate = rotate;
    cmd.solid_color = fimg->fillcolor;

    if (fimg->srcAddr != NULL && (fimg->srcW != fimg->dstW || fimg->srcH != fimg->dstH)) {
        scale.mode = filterSkiaToDriver[fimg->isFilter];
        scale.factor = SCALING_PIXELS;
        scale.src_w = fimg->srcW;
        scale.src_h = fimg->srcH;
        scale.dst_w = fimg->dstW;
        scale.dst_h = fimg->dstH;
        cmd.scaling = &scale;
    } else if (fimg->mskAddr != NULL && (fimg->mskW != fimg->dstW || fimg->mskH != fimg->dstH)) {
        scale.mode = filterSkiaToDriver[fimg->isFilter];
        scale.factor = SCALING_PIXELS;
        scale.src_w = fimg->mskW;
        scale.src_h = fimg->mskH;
        scale.dst_w = fimg->dstW;
        scale.dst_h = fimg->dstH;
        cmd.scaling = &scale;
    } else
        cmd.scaling = NULL;

    cmd.repeat = NULL;
    cmd.bluscr = NULL;

    if (fimg->srcAddr != NULL) {
        srcImage.addr.type = ADDR_USER;
        srcImage.addr.start = (long unsigned)fimg->srcAddr;
        srcImage.addr.size = fimg->srcFWStride * fimg->srcFH;
        srcImage.addr.cacheable = true;
        srcImage.addr.pinnable = false;
        srcImage.width = fimg->srcFWStride / fimg->srcBPP;
        srcImage.height = fimg->srcFH;
        srcImage.stride = fimg->srcFWStride;
        if (fimg->srcColorFormat == SkBitmap::kRGB_565_Config)
            srcImage.order = AX_RGB;
        else if (fimg->srcColorFormat == SkBitmap::kARGB_4444_Config)
            srcImage.order = RGB_AX;
        else
            srcImage.order = AX_BGR; // kARGB_8888_Config

        srcImage.fmt = formatSkiaToDriver[fimg->srcColorFormat];

        cmd.src = &srcImage;
        srcRect = {fimg->srcX, fimg->srcY, fimg->srcX + fimg->srcW, fimg->srcY + fimg->srcH};
        cmd.src_rect = &srcRect;
    } else {
        cmd.src = NULL;
        cmd.src_rect = NULL;
    }

    if (fimg->dstAddr != NULL) {
        dstImage.addr.type = ADDR_USER;
        dstImage.addr.start = (long unsigned)fimg->dstAddr;
        dstImage.addr.size = fimg->dstFWStride * fimg->dstFH;
        dstImage.addr.cacheable = true;
        dstImage.addr.pinnable = false;
        dstImage.width = fimg->dstFWStride / fimg->dstBPP;
        dstImage.height = fimg->dstFH;
        dstImage.stride = fimg->dstFWStride;
        if (fimg->dstColorFormat == SkBitmap::kRGB_565_Config)
            dstImage.order = AX_RGB;
        else if (fimg->dstColorFormat == SkBitmap::kARGB_4444_Config)
            dstImage.order = RGB_AX;
        else
            dstImage.order = AX_BGR; // kARGB_8888_Config

        dstImage.fmt = formatSkiaToDriver[fimg->dstColorFormat];

        cmd.dst = &dstImage;
        dstRect = {fimg->dstX, fimg->dstY, fimg->dstX + fimg->dstW, fimg->dstY + fimg->dstH};
        cmd.dst_rect = &dstRect;
    } else {
        cmd.dst = NULL;
        cmd.dst_rect = NULL;
    }

    if (fimg->mskAddr != NULL) {
        mskImage.addr.type = ADDR_USER;
        mskImage.addr.start = (long unsigned)fimg->mskAddr;
        mskImage.addr.size = fimg->mskFWStride * fimg->mskFH;
        mskImage.addr.cacheable = true;
        mskImage.addr.pinnable = false;
        mskImage.width = fimg->mskW;
        mskImage.height = fimg->mskFH;
        mskImage.stride = fimg->mskFWStride,
        mskImage.order = AX_RGB;
        mskImage.fmt = formatSkiaToDriver[fimg->mskColorFormat];

        cmd.msk = &mskImage;
        mskRect = {fimg->mskX, fimg->mskY, fimg->mskX + fimg->mskW, fimg->mskY + fimg->mskH};
        cmd.msk_rect = &mskRect;
    } else {
        cmd.msk = NULL;
        cmd.msk_rect = NULL;
    }

    cmd.seq_no = seq_no++;

#if FIMGAPI_HYBRID_USE
    int ratio = FimgApiCheckPossibleHybrid(fimg);
    if (ratio == 0) {
        ret = FIMGAPI_FINISHED;
    } else {
        fimg->clipT = fimg->clipT + ((fimg->clipB - fimg->clipT) / 10) * (10 - ratio);
        ret = FIMGAPI_HYBRID;
    }
#else
    ret = FIMGAPI_FINISHED;
#endif

#if (FIMGAPI_COPROMISE_USE == FALSE)
    if (FimgApiCheckPossible_Clipping(fimg) == false)
        return false;
#endif

    clip.enable = true;
    clip.y1 = fimg->clipT;
    clip.y2 = fimg->clipB;
    clip.x1 = fimg->clipL;
    clip.x2 = fimg->clipR;

    cmd.clipping = &clip;

#if defined(FIMGAPI_DEBUG_MESSAGE)
    printDataBlit("Before stretchFimgApi:", &cmd);
    printDataMatrix(fimg->matrixType);
#endif

    if (stretchFimgApi(&cmd) < 0) {
#if defined(FIMGAPI_DEBUG_MESSAGE)
        LOGE("%s:stretch failed\n", __FUNCTION__);
#endif
        return false;
    }

    return ret;
}

int FimgApiSync(const char *func_name)
{
    if (SyncFimgApi() < 0)
        return false;

    return true;
}

int FimgARGB32_BW(struct Fimg &fimg, const SkBitmap &device, const SkMask &mask, const SkIRect &clip,
                  const SkPMColor &color, const unsigned int alpha)
{
    int x = clip.fLeft;
    int y = clip.fTop;
    int width = clip.width();
    int height = clip.height();

    fimg.srcAddr        = (unsigned char *)NULL;

    fimg.isSolidFill    = false;
    fimg.fillcolor      = toARGB32(color);

    fimg.srcColorFormat = device.config();

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = device.rowBytes();
    fimg.dstFH          = device.height();
    fimg.dstBPP         = device.bytesPerPixel();
    fimg.dstColorFormat = device.config();
    fimg.dstAddr        = (unsigned char *)device.getAddr(0,0);

    fimg.clipT = y;
    fimg.clipB = y + height;
    fimg.clipL = x;
    fimg.clipR = x + width;

    fimg.mskX           = 0;
    fimg.mskY           = 0;
    fimg.mskW           = width;
    fimg.mskH           = height;
    fimg.mskFWStride    = mask.fRowBytes;
    fimg.mskFH          = height;
    fimg.mskBPP         = 1;
    fimg.mskColorFormat = SkBitmap::kA1_Config;
    fimg.mskAddr        = (unsigned char *)mask.fImage;

    fimg.rotate         = 0;

    fimg.xfermode = SkXfermode::kSrcOver_Mode;
    fimg.isDither = false;
    fimg.colorFilter = NULL;
    fimg.matrixType = 0;
    fimg.matrixSx = 0;
    fimg.matrixSy = 0;

    fimg.alpha = 0xFF;

    fimg.canusehybrid = 0;

    return FimgApiStretch(&fimg, __func__);
}

int FimgARGB32_Rect(struct Fimg &fimg, uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    fimg.srcAddr        = (unsigned char *)NULL;
    fimg.mskAddr        = (unsigned char *)NULL;

    fimg.isSolidFill    = false;
    fimg.fillcolor      = toARGB32(color);

    fimg.srcColorFormat = SkBitmap::kARGB_8888_Config;

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = rowbyte;
    fimg.dstFH          = y + height;
    fimg.dstBPP         = 4;
    fimg.dstColorFormat = SkBitmap::kARGB_8888_Config;
    fimg.dstAddr        = (unsigned char *)device;

    fimg.clipT = y;
    fimg.clipB = y + height;
    fimg.clipL = x;
    fimg.clipR = x + width;

    fimg.rotate         = 0;

    fimg.xfermode = SkXfermode::kSrcOver_Mode;
    fimg.isDither = false;
    fimg.colorFilter = NULL;
    fimg.matrixType = 0;
    fimg.matrixSx = 0;
    fimg.matrixSy = 0;

    fimg.alpha = 0xFF;

    fimg.canusehybrid = 0;

    return FimgApiStretch(&fimg, __func__);
}

uint32_t toARGB32(uint32_t color)
{
    U8CPU a = SkGetPackedA32(color);
    U8CPU r = SkGetPackedR32(color);
    U8CPU g = SkGetPackedG32(color);
    U8CPU b = SkGetPackedB32(color);

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}
#endif
