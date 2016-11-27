
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
#include "SkFimgV4L2.h"
#include "SkUtils.h"
#define FLOAT_TO_INT_PRECISION (14)

struct bl_property default_prop = { DEV_G2D0, false };

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
    if (fimg->dstAddr != NULL) {
        switch (fimg->dstColorFormat) {
            case kRGB_565_SkColorType:
                break;
            case kN32_SkColorType:
                break;
            default:
                return false;
        }
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

    if (fimg->srcFWStride <= 0 || fimg->srcBPP <= 0)
        return false;

    if (fimg->dstFWStride <= 0 || fimg->dstBPP <= 0)
        return false;

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

bool FimgApiCompromise(Fimg *fimg)
{
    if ((fimg->clipR-fimg->clipL) * (fimg->clipB-fimg->clipT) < 480 * 480)
        return false;
    return true;
}

int ChangeColorForV4L2(int colorFormat) {
    if (colorFormat == kRGB_565_SkColorType)
        return V4L2_PIX_FMT_RGB565;
    if (colorFormat == kN32_SkColorType)
        return V4L2_PIX_FMT_RGB32;
    return -1;
}

int FimgApiStretch(Fimg *fimg, const char *func_name)
{
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
    if (FimgApiCheckPossible_Clipping(fimg) == false)
        return false;
    enum rotation rotate;

    switch (fimg->rotate) {
    case 0:
        rotate = ROT_ORIGIN;
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

    bl_handle_t fimg_handle_t = NULL;
    int result = 0;
    void *addr[BL_MAX_PLANES];
    fimg_handle_t = exynos_bl_create(&default_prop);
    if (!fimg_handle_t) {
        ALOGE("<SkFimgV4L2> exynos_bl_create error");
        return false;
    }

    result = exynos_bl_set_src_format(fimg_handle_t, (fimg->srcFWStride)/(fimg->srcBPP), fimg->srcFH, fimg->srcX, fimg->srcY, fimg->srcW, fimg->srcH, ChangeColorForV4L2(fimg->srcColorFormat));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_src_format error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_dst_format(fimg_handle_t, (fimg->dstFWStride)/(fimg->dstBPP), fimg->dstFH, fimg->dstX, fimg->dstY, fimg->dstW, fimg->dstH, ChangeColorForV4L2(fimg->dstColorFormat));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_format error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    addr[0] = (void *)fimg->srcAddr;
    addr[1] = NULL;
    addr[2] = NULL;

    result = exynos_bl_set_src_addr(fimg_handle_t, addr, V4L2_MEMORY_USERPTR);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_src_addr error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    addr[0] = (void *)fimg->dstAddr;

    result = exynos_bl_set_dst_addr(fimg_handle_t, addr, V4L2_MEMORY_USERPTR);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_addr error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_clip(fimg_handle_t, true, fimg->clipL, fimg->clipT, fimg->clipR - fimg->clipL, fimg->clipB - fimg->clipT);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_clip error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }
    if (fimg->isDither) {
        result = exynos_bl_set_dither(fimg_handle_t, true);
        if (result < 0) {
            ALOGE("<SkFimgV4L2> exynos_bl_set_dither : %d", result);
            exynos_bl_destroy(fimg_handle_t);
            return false;
        }
    }

    result = exynos_bl_set_blend(fimg_handle_t, (BL_OP_TYPE)(fimg->xfermode+1), true);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_galpha(fimg_handle_t, (fimg->alpha != 0xFF), fimg->alpha);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_galpha error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    if ((fimg->srcAddr != NULL) && (fimg->srcW == fimg->dstW) && (fimg->srcH == fimg->dstH))
        fimg->isFilter = -1;

    if (!(fimg->matrixType && SkMatrix::kScale_Mask))
        fimg->isFilter = -1;

    result = exynos_bl_set_scale(fimg_handle_t, (BL_SCALE)(fimg->isFilter + 1), fimg->srcW, fimg->dstW, fimg->srcH, fimg->dstH);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_scale error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    if (fimg->isFilter == -1)
        result = exynos_bl_set_repeat(fimg_handle_t, (BL_REPEAT)fimg->repeat);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_repeat error : %d", result);
        return false;
    }

    /*result = exynos_bl_set_rotate(fimg_handle_t, rotate, false, false);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_rotate error : %d", result);
        return false;
    }*/

    result = exynos_bl_do_blend(fimg_handle_t);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_do_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    exynos_bl_destroy(fimg_handle_t);

#if defined(FIMGAPI_DEBUG_MESSAGE)
    printDataBlit("Before stretchFimgApi:", &cmd);
#endif

    return FIMGAPI_FINISHED;
}

int FimgARGB32_Rect(const uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    bl_handle_t fimg_handle_t = NULL;
    int result = 0;
    void *addr[BL_MAX_PLANES];

    return false;

    if ((x < 0) || (y < 0) || (width <= 0) || (height <= 0) || (rowbyte <= 0))
        return false;

    fimg_handle_t = exynos_bl_create(&default_prop);
    if (!fimg_handle_t) {
        ALOGE("<SkFimgV4L2> exynos_bl_create error");
        return false;
    }

    result = exynos_bl_set_color_fill(fimg_handle_t, true, toARGB32(color));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_color_fill error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_galpha(fimg_handle_t, false, 0xFF);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_galpha error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    addr[0] = (void *)device;
    addr[1] = NULL;
    addr[2] = NULL;

    result = exynos_bl_set_dst_addr(fimg_handle_t, addr, V4L2_MEMORY_USERPTR);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_addr error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_dst_format(fimg_handle_t, rowbyte/32, y+height, x, y, width, height, ChangeColorForV4L2(kN32_SkColorType));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_format error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_blend(fimg_handle_t, OP_SRC_OVER, PREMULTIPLIED);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_do_blend(fimg_handle_t);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_do_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    exynos_bl_destroy(fimg_handle_t);

    return FIMGAPI_FINISHED;
}

int FimgRGB16_Rect(const uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    bl_handle_t fimg_handle_t = NULL;
    int result = 0;
    void *addr[BL_MAX_PLANES];

    return false;

    if ((x < 0) || (y < 0) || (width <= 0) || (height <= 0) || (rowbyte <= 0))
        return false;

    fimg_handle_t = exynos_bl_create(&default_prop);
    if (!fimg_handle_t) {
        ALOGE("<SkFimgV4L2> exynos_bl_create error");
        return false;
    }

    result = exynos_bl_set_color_fill(fimg_handle_t, true, toARGB32(color));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_color_fill error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_galpha(fimg_handle_t, false, 0xFF);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_galpha error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    addr[0] = (void *)device;
    addr[1] = NULL;
    addr[2] = NULL;

    result = exynos_bl_set_dst_addr(fimg_handle_t, addr, V4L2_MEMORY_USERPTR);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_addr error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_dst_format(fimg_handle_t, rowbyte/16, y+height, x, y, width, height, ChangeColorForV4L2(kRGB_565_SkColorType));
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_dst_format error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_set_blend(fimg_handle_t, OP_SRC_OVER, PREMULTIPLIED);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_set_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    result = exynos_bl_do_blend(fimg_handle_t);
    if (result < 0) {
        ALOGE("<SkFimgV4L2> exynos_bl_do_blend error : %d", result);
        exynos_bl_destroy(fimg_handle_t);
        return false;
    }

    exynos_bl_destroy(fimg_handle_t);

    return FIMGAPI_FINISHED;
}
uint32_t toARGB32(uint32_t color)
{
    U8CPU a = SkGetPackedA32(color);
    U8CPU r = SkGetPackedR32(color);
    U8CPU g = SkGetPackedG32(color);
    U8CPU b = SkGetPackedB32(color);

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}
