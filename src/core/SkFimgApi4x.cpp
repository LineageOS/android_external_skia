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

#define LOG_TAG "SKIA"
#include <cutils/log.h>
#include <stdlib.h>
#include "SkFimgApi4x.h"
#include "SkUtils.h"
#define FLOAT_TO_INT_PRECISION (14)

enum color_format formatSkiaToDriver[] = {
    SRC_DST_FORMAT_END, //!< bitmap has not been configured
    SRC_DST_FORMAT_END, //!< Mask 1bit is not supported by FIMG2D
    SRC_DST_FORMAT_END, //!< Mask 8bit is not supported by FIMG2D
    SRC_DST_FORMAT_END, //!< kIndex8_Config is not supported by FIMG2D
    CF_RGB_565,
    SRC_DST_FORMAT_END, //!< ARGB4444 is not supported by FIMG2D
    CF_ARGB_8888,
};

enum blit_op blendingSkiaToDriver[] = {
    BLIT_OP_CLR,
    BLIT_OP_SRC,
    BLIT_OP_DST,
    BLIT_OP_SRC_OVER,
};

enum scaling filterSkiaToDriver[] = {
    SCALING_NEAREST,
    SCALING_BILINEAR,
};

bool FimgApiCheckPossible(Fimg *fimg)
{
    if (fimg->srcAddr != NULL) {
        switch (fimg->srcColorFormat) {
        case kRGB_565_SkColorType:
        case kN32_SkColorType:
            break;
        default:
            return false;
        }
    }

    switch (fimg->dstColorFormat) {
    case kRGB_565_SkColorType:
        break;
    case kN32_SkColorType:
        break;
    default:
        return false;
    }

    switch (fimg->xfermode) {
    case SkXfermode::kSrcOver_Mode:
    case SkXfermode::kClear_Mode:
    case SkXfermode::kSrc_Mode:
    case SkXfermode::kDst_Mode:
        break;
    default:
        return false;
    }

    if (fimg->colorFilter != 0)
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
    struct compromise_param param;

    /* source format setting*/
    switch (fimg->srcColorFormat) {
        case kRGB_565_SkColorType:
            param.src_fmt = 0;
            break;
        case kN32_SkColorType:
            param.src_fmt = 1;
            break;
        case kUnknown_SkColorType:
            param.src_fmt = 2;
            break;
        default :
            return false;
    }
    /* destination format setting */
    switch (fimg->dstColorFormat) {
        case kRGB_565_SkColorType:
            param.dst_fmt = 0;
            break;
        case kN32_SkColorType:
            param.dst_fmt = 1;
            break;
        default :
            return false;
    }
    /* scaling setting */
    if (fimg->srcW == fimg->dstW && fimg->srcH == fimg->dstH)
        param.isScaling = 0;
    else if (fimg->srcW * fimg->srcH < fimg->dstW * fimg->dstH)
        param.isScaling = 1;
    else
        param.isScaling = 2;
    /* filter_mode setting */
    param.isFilter = fimg->isFilter;
    /* blending mode setting */
    if (fimg->xfermode == SkXfermode::kSrc_Mode)
        param.isSrcOver = 0;
    else
        param.isSrcOver = 1;

    param.clipW = (fimg->clipR - fimg->clipL) * 1.2;
    param.clipH = (fimg->clipB - fimg->clipT) * 0.8;

    return compromiseFimgApi(&param);
}

bool FimgSupportNegativeCoordinate(Fimg *fimg)
{
    unsigned int dstL, dstR, dstT, dstB;
    int dstFW, dstFH;

    if (fimg->dstBPP <= 0)
        return false;

    if (fimg->dstX < 0) {
        if ((fimg->dstW + fimg->dstX) < (fimg->dstFWStride / fimg->dstBPP))
            dstFW = fimg->dstW + fimg->dstX;
        else
            dstFW = fimg->dstFWStride / fimg->dstBPP;

        dstL = ((unsigned int)(0 - fimg->dstX) << FLOAT_TO_INT_PRECISION) / fimg->dstW;
        dstR = ((unsigned int)(dstFW - fimg->dstX) << FLOAT_TO_INT_PRECISION) / fimg->dstW;
        fimg->srcX = (int)((fimg->srcW * dstL + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) + fimg->srcX;
        fimg->srcW = (int)((fimg->srcW * dstR + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) - fimg->srcX;
        fimg->dstW = dstFW;
        fimg->dstX = 0;
    }

    if (fimg->dstY < 0) {
        if ((fimg->dstH + fimg->dstY) < fimg->dstFH)
            dstFH = fimg->dstH + fimg->dstY;
        else
            dstFH = fimg->dstFH;

        dstT = ((unsigned int)(0 - fimg->dstY) << FLOAT_TO_INT_PRECISION) / fimg->dstH;
        dstB = ((unsigned int)(dstFH - fimg->dstY) << FLOAT_TO_INT_PRECISION) / fimg->dstH;
        fimg->srcY = (int)((fimg->srcH * dstT + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) + fimg->srcY;
        fimg->srcH = (int)((fimg->srcH * dstB + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) - fimg->srcY;
        fimg->dstH = dstFH;
        fimg->dstY = 0;
    }

    return true;
}

int FimgApiStretch(Fimg *fimg, const char *func_name)
{
    static unsigned int seq_no = 100;

    struct fimg2d_blit cmd;
    struct fimg2d_image srcImage;
    struct fimg2d_image dstImage;

    /* to support negative coordinate */
    if ((fimg->dstAddr != NULL) && ((fimg->dstX < 0) || (fimg->dstY < 0))) {
        if (!FimgSupportNegativeCoordinate(fimg))
            return false;
    }

    if (FimgApiCheckPossible(fimg) == false)
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

#if FIMGAPI_COMPROMISE_USE
    if (FimgApiCompromise(fimg) == false)
        return false;
#endif
    enum rotation rotate;

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
    cmd.param.g_alpha = fimg->alpha;
    cmd.param.premult = PREMULTIPLIED;
    cmd.param.dither = fimg->isDither;
    cmd.param.rotate = rotate;
    cmd.param.solid_color = fimg->fillcolor;

    if (fimg->srcAddr != NULL && (fimg->srcW != fimg->dstW || fimg->srcH != fimg->dstH)) {
        cmd.param.scaling.mode = filterSkiaToDriver[fimg->isFilter];
        cmd.param.scaling.src_w = fimg->srcW;
        cmd.param.scaling.src_h = fimg->srcH;
        cmd.param.scaling.dst_w = fimg->dstW;
        cmd.param.scaling.dst_h = fimg->dstH;
    } else
        cmd.param.scaling.mode = NO_SCALING;

    cmd.param.repeat.mode = NO_REPEAT;
    cmd.param.repeat.pad_color = 0x0;

    cmd.param.bluscr.mode = OPAQUE;
    cmd.param.bluscr.bs_color = 0x0;
    cmd.param.bluscr.bg_color = 0x0;

    if (fimg->srcAddr != NULL) {
        srcImage.addr.type = ADDR_USER;
        srcImage.addr.start = (long unsigned)fimg->srcAddr;
        srcImage.plane2.type = ADDR_USER;
        srcImage.plane2.start = NULL;
        srcImage.need_cacheopr = true;
        if (fimg->srcFWStride <= 0 || fimg->srcBPP <= 0)
            return false;
        srcImage.width = fimg->srcFWStride / fimg->srcBPP;
        srcImage.height = fimg->srcFH;
        srcImage.stride = fimg->srcFWStride;
        if (fimg->srcColorFormat == kRGB_565_SkColorType)
            srcImage.order = AX_RGB;
        else
            srcImage.order = AX_BGR; // kARGB_8888_Config

        srcImage.fmt = formatSkiaToDriver[fimg->srcColorFormat];
        srcImage.rect.x1 = fimg->srcX;
        srcImage.rect.y1 = fimg->srcY;
        srcImage.rect.x2 = fimg->srcX + fimg->srcW;
        srcImage.rect.y2 = fimg->srcY + fimg->srcH;
        cmd.src = &srcImage;
    } else
        cmd.src = NULL;

    if (fimg->dstAddr != NULL) {
        dstImage.addr.type = ADDR_USER;
        dstImage.addr.start = (long unsigned)fimg->dstAddr;
        dstImage.plane2.type = ADDR_USER;
        dstImage.plane2.start = NULL;
        dstImage.need_cacheopr = true;
        if (fimg->dstFWStride <= 0 || fimg->dstBPP <= 0)
            return false;
        dstImage.width = fimg->dstFWStride / fimg->dstBPP;
        dstImage.height = fimg->dstFH;
        dstImage.stride = fimg->dstFWStride;
        if (fimg->dstColorFormat == kRGB_565_SkColorType)
            dstImage.order = AX_RGB;
        else
            dstImage.order = AX_BGR; // kARGB_8888_Config

        dstImage.fmt = formatSkiaToDriver[fimg->dstColorFormat];
        dstImage.rect.x1 = fimg->dstX;
        dstImage.rect.y1 = fimg->dstY;
        dstImage.rect.x2 = fimg->dstX + fimg->dstW;
        dstImage.rect.y2 = fimg->dstY + fimg->dstH;

        cmd.param.clipping.enable = true;
        cmd.param.clipping.x1 = fimg->clipL;
        cmd.param.clipping.y1 = fimg->clipT;
        cmd.param.clipping.x2 = fimg->clipR;
        cmd.param.clipping.y2 = fimg->clipB;

        cmd.dst = &dstImage;

    } else
        cmd.dst = NULL;

    cmd.msk = NULL;

    cmd.tmp = NULL;
    cmd.sync = BLIT_SYNC;
    cmd.seq_no = seq_no++;

    if (FimgApiCheckPossible_Clipping(fimg) == false)
        return false;

#if defined(FIMGAPI_DEBUG_MESSAGE)
    printDataBlit("Before stretchFimgApi:", &cmd);
#endif

    if (stretchFimgApi(&cmd) < 0) {
#if defined(FIMGAPI_DEBUG_MESSAGE)
        ALOGE("%s:stretch failed\n", __FUNCTION__);
#endif
        return false;
    }
    return FIMGAPI_FINISHED;
}

int FimgARGB32_Rect(const uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    Fimg fimg;
    memset(&fimg, 0, sizeof(Fimg));

    fimg.srcAddr        = (unsigned char *)NULL;
    fimg.srcColorFormat = kUnknown_SkColorType;
    fimg.mskAddr        = (unsigned char *)NULL;

    fimg.fillcolor      = toARGB32(color);

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = rowbyte;
    fimg.dstFH          = y + height;
    fimg.dstBPP         = 4; /* 4Byte */
    fimg.dstColorFormat = kN32_SkColorType;
    fimg.dstAddr        = (unsigned char *)device;

    fimg.clipT          = y;
    fimg.clipB          = y + height;
    fimg.clipL          = x;
    fimg.clipR          = x + width;

    fimg.rotate         = 0;

    fimg.xfermode       = SkXfermode::kSrcOver_Mode;
    fimg.isDither       = false;
    fimg.colorFilter    = 0;
    fimg.matrixType     = 0;
    fimg.matrixSx       = 0;
    fimg.matrixSy       = 0;
    fimg.alpha          = 0xFF;

    return FimgApiStretch(&fimg, __func__);
}

int FimgRGB16_Rect(const uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    Fimg fimg;
    memset(&fimg, 0, sizeof(Fimg));

    fimg.srcAddr        = (unsigned char *)NULL;
    fimg.srcColorFormat = kUnknown_SkColorType;
    fimg.mskAddr        = (unsigned char *)NULL;

    fimg.fillcolor      = toARGB32(color);

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = rowbyte;
    fimg.dstFH          = y + height;
    fimg.dstBPP         = 2;
    fimg.dstColorFormat = kRGB_565_SkColorType;
    fimg.dstAddr        = (unsigned char *)device;

    fimg.clipT          = y;
    fimg.clipB          = y + height;
    fimg.clipL          = x;
    fimg.clipR          = x + width;

    fimg.rotate         = 0;

    fimg.xfermode       = SkXfermode::kSrcOver_Mode;
    fimg.isDither       = false;
    fimg.colorFilter    = 0;
    fimg.matrixType     = 0;
    fimg.matrixSx       = 0;
    fimg.matrixSy       = 0;
    fimg.alpha          = 0xFF;

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
