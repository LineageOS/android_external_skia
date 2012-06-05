/*
 * Copyright 2011, Samsung Electronics Co. LTD
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

#include "csc_swap_neon.h"
#include "SecJpegDecoderForSkia.h"

#define JPEG_ERROR_LOG(fmt,...)

int SecJpegDecoderForSkia::create()
{
    if(m_pcJpegDec) {
        return ERROR_ALREADY_CREATE;
    }

    m_pcJpegDec = new SecJpegDecoderHal;

    if(!m_pcJpegDec) {
        JPEG_ERROR_LOG("[%s] create failed",__func__);
        return ERROR_CANNOT_CREATE_SEC_JPEG_DEC_HAL;
    }

    int iRet = m_pcJpegDec->create();
    if(iRet) {
        JPEG_ERROR_LOG("[%s] create failed",__func__);
        destroy();
        return iRet;
    }

    iRet = m_pcJpegDec->setCache(JPEG_CACHE_ON);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] cache setting fail",__func__);
        destroy();
        return iRet;
    }

    return ERROR_NONE;
}

int SecJpegDecoderForSkia::destroy()
{
    if(m_pcJpegDec == NULL) {
        return ERROR_ALREADY_DESTROY;
    }

    m_pcJpegDec->destroy();

    delete m_pcJpegDec;
    m_pcJpegDec = NULL;

    if (m_pcTmpInBuf) {
        free(m_pcTmpInBuf);
        m_pcTmpInBuf = NULL;
    }
    if (m_pcTmpOutBuf) {
        free(m_pcTmpOutBuf);
        m_pcTmpOutBuf = NULL;
    }

    return ERROR_NONE;
}

SecJpegDecoderForSkia::SecJpegDecoderForSkia() :
    m_bHwJpegFlag   (false)
{
    m_pcJpegDec = NULL;
    m_pcJpegInBuf = NULL;
    m_pcJpegOutBuf = NULL;
    m_iOutImageSize = 0;
    m_iInBufSize = 0;
    m_iOutBufSize = 0;
    m_pcTmpInBuf = NULL;
    m_pcTmpOutBuf = NULL;
}

SecJpegDecoderForSkia::~SecJpegDecoderForSkia()
{
    if(m_pcJpegDec) {
        destroy();
    }
}

bool SecJpegDecoderForSkia::checkHwJPEGSupport(SkBitmap::Config config, jpeg_decompress_struct *cinfo, int sampleSize)
{
    bool ret = 0;
    int scalingratio = 1;

    if (sampleSize == 1) {
        if ((cinfo->image_width % 2 == 0) && (cinfo->image_height % 2 == 0)) {
            m_bHwJpegFlag = true;
        } else {
            m_bHwJpegFlag = false;
        }
    } else if (sampleSize >= 4) {
        if (((cinfo->image_width / 4) % 2 == 0) && ((cinfo->image_height / 4) % 2 == 0)) {
            m_bHwJpegFlag = true;
            scalingratio = 4;
        } else {
            m_bHwJpegFlag = false;
        }
    } else if (sampleSize >= 2) {
        if (((cinfo->image_width / 2) % 2 == 0) && ((cinfo->image_height / 2) % 2 == 0)) {
            m_bHwJpegFlag = true;
            scalingratio = 2;
        } else {
            m_bHwJpegFlag = false;
        }
    } else {
        m_bHwJpegFlag = false;
    }

    if ((cinfo->image_width <= JPEG_HW_MIN_WIDTH) || (cinfo->image_height <= JPEG_HW_MIN_HEIGHT)) {
        m_bHwJpegFlag = false;
    }

    if ((m_bHwJpegFlag == true) &&
        (cinfo->progressive_mode == false) &&
        ((config == SkBitmap::kARGB_8888_Config) ||
        (config == SkBitmap::kRGB_565_Config))) {
        m_bHwJpegFlag = true;
        cinfo->output_width = cinfo->image_width / scalingratio;
        cinfo->output_height = cinfo->image_height / scalingratio;
    } else {
            m_bHwJpegFlag = false;
    }

    return m_bHwJpegFlag;
}

int SecJpegDecoderForSkia::setDecodeHwJPEGConfig(SkBitmap::Config config, jpeg_decompress_struct *cinfo, SkStream *stream)
{
    int iOutFmt = V4L2_PIX_FMT_RGB32;
    int iJpegSize = 0;
    int iRet = ERROR_NONE;
    char *pcTmpInBuf = NULL;
    char *pcTmpOutBuf = NULL;
    char *pcJpegInBuf = NULL;
    char *pcJpegOutBuf = NULL;
    int iInBufSize = 0;
    int iOutBufSize = 0;
    int iPixelSize = 0;
    int iTempFlagForHwJpeg = 0;

    if(m_pcJpegDec == NULL) {
        iRet = create();
        if (iRet) {
            JPEG_ERROR_LOG("[%s]JPEG Decode Init failed", __func__);
            return iRet;
        }
    }

    switch (config) {
        case SkBitmap::kARGB_8888_Config:
            iOutFmt = V4L2_PIX_FMT_RGB32;
            iPixelSize = 4;
            break;
        case SkBitmap::kRGB_565_Config:
            iOutFmt = V4L2_PIX_FMT_RGB565X;
            iPixelSize = 2;
            break;
        default:
            return ERROR_INVALID_COLOR_FORMAT;
            break;
    }

    iInBufSize = cinfo->image_width*cinfo->image_height*iPixelSize;
    if (iInBufSize <= 0) {
        return ERROR_BUFFER_TOO_SMALL;
    }

    iOutBufSize = m_iOutImageSize = cinfo->output_width*cinfo->output_height*iPixelSize;
    if (iOutBufSize <= 0) {
        return ERROR_BUFFER_TOO_SMALL;
    }

    iJpegSize = stream->getLength();
    if (iJpegSize <= 0) {
        return ERROR_JPEG_SIZE_TOO_SMALL;
    }

    iRet = m_pcJpegDec->setSize(cinfo->image_width, cinfo->image_height);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] setSize Failed", __func__);
        return iRet;
    }

    iRet = m_pcJpegDec->setScaledSize(cinfo->output_width, cinfo->output_height);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] setScaledSize Failed", __func__);
        return iRet;
    }

    iRet = m_pcJpegDec->setJpegSize(iJpegSize);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] setJpegSize Failed", __func__);
        return iRet;
    }

    iRet = m_pcJpegDec->setJpegFormat(V4L2_PIX_FMT_JPEG_422);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] setJpegFormat Failed", __func__);
        return iRet;
    }

    iRet = m_pcJpegDec->setColorFormat(iOutFmt);
    if(iRet) {
        JPEG_ERROR_LOG("[%s] setColorFormat Failed", __func__);
        return iRet;
    }

    if (stream->getMemoryBase() == NULL) {
        iTempFlagForHwJpeg = 0;
        if (stream->rewind() == false) {
            m_bHwJpegFlag = false;
            return ERROR_REWIND_FAIL;
        }
        pcTmpInBuf = (char *)malloc(iInBufSize+JPEG_WA_BUFFER_SIZE);
        pcJpegInBuf = ((int)pcTmpInBuf%JPEG_BYTE_ALIGN)? \
            pcTmpInBuf + JPEG_BYTE_ALIGN - ((int)pcTmpInBuf%JPEG_BYTE_ALIGN):pcTmpInBuf;
    } else if ((int)(stream->getMemoryBase())%JPEG_BYTE_ALIGN) {
        iTempFlagForHwJpeg = 1;
        pcTmpInBuf = (char *)malloc(iInBufSize+JPEG_WA_BUFFER_SIZE);
        pcJpegInBuf = ((int)pcTmpInBuf%JPEG_BYTE_ALIGN)? \
            pcTmpInBuf + JPEG_BYTE_ALIGN - ((int)pcTmpInBuf%JPEG_BYTE_ALIGN):pcTmpInBuf;
    } else {
        iTempFlagForHwJpeg = 2;
        pcJpegInBuf = (char *)(stream->getMemoryBase());
        iInBufSize = iJpegSize;
    }

    if (m_pcJpegDec->setInBuf(pcJpegInBuf, iInBufSize) != ERROR_NONE) {
        JPEG_ERROR_LOG("[%s] FAIL SetInBuf", __func__);
        return ERROR_SET_IN_BUF_FAIL;
    }

    if ((int)m_pcJpegOutBuf%JPEG_BYTE_ALIGN) {
        pcTmpOutBuf = (char *)malloc(iOutBufSize+JPEG_WA_BUFFER_SIZE);
        pcJpegOutBuf = m_pcJpegOutBuf = ((int)pcTmpOutBuf%JPEG_BYTE_ALIGN)? \
            pcTmpOutBuf + JPEG_BYTE_ALIGN - ((int)pcTmpOutBuf%JPEG_BYTE_ALIGN):pcTmpOutBuf;
    } else {
        pcJpegOutBuf = (char *)m_pcJpegOutBuf;
        iOutBufSize = m_iOutBufSize;
    }

    if (m_pcJpegDec->setOutBuf(&pcJpegOutBuf, &iOutBufSize) != ERROR_NONE) {
        JPEG_ERROR_LOG("[%s] FAIL setOutBuf", __func__);
        return ERROR_SET_OUT_BUF_FAIL;
    }

    iRet = m_pcJpegDec->updateConfig();
    if(iRet) {
        JPEG_ERROR_LOG("[%s] updateConfig Failed", __func__);
        return iRet;
    }

    switch(iTempFlagForHwJpeg) {
        case 0:
            iJpegSize = stream->read((void *)pcJpegInBuf,iJpegSize);
            break;
        case 1:
            memcpy(pcJpegInBuf, (char *)(stream->getMemoryBase()), iJpegSize);
            break;
        case 2:
            break;
        default:
            break;
    }

    m_pcTmpInBuf = pcTmpInBuf;
    m_pcTmpOutBuf - pcTmpOutBuf;

    return ERROR_NONE;
}

int SecJpegDecoderForSkia::setHwJPEGInputBuffer(char *pcBuffer, int iSize)
{
    int iRet = ERROR_NONE;

    if(m_pcJpegDec == NULL) {
        iRet = create();
        if (iRet) {
            JPEG_ERROR_LOG("[%s]JPEG Decode Init failed", __func__);
            return iRet;
        }
    }

    if (iSize <=0) {
        return ERROR_BUFFER_TOO_SMALL;
    }

    m_pcJpegInBuf = pcBuffer;
    m_iInBufSize = iSize;

    return ERROR_NONE;
}

int SecJpegDecoderForSkia::setHwJPEGOutputBuffer(char *pcBuffer, int iSize)
{
    int iRet = ERROR_NONE;

    if(m_pcJpegDec == NULL) {
        iRet = create();
        if (iRet) {
            JPEG_ERROR_LOG("[%s]JPEG Decode Init failed", __func__);
            return iRet;
        }
    }

    if (pcBuffer == NULL) {
        return ERROR_BUFFR_IS_NULL;
    }

    if (iSize <=0) {
        return ERROR_BUFFER_TOO_SMALL;
    }

    m_pcJpegOutBuf = pcBuffer;
    m_iOutBufSize = iSize;

    return ERROR_NONE;
}

int SecJpegDecoderForSkia::executeDecodeHwJPEG(void)
{
    return m_pcJpegDec->decode();
}

void SecJpegDecoderForSkia::outputJpeg(SkBitmap *bm, unsigned char *rowptr)
{
    char out_tmp[4], rowptr_tmp[4];
    const void *jpeg_out;
    int iOutputSize = 0;

    jpeg_out = (const void *)m_pcJpegOutBuf;
    iOutputSize = m_iOutImageSize;

    //csc_swap_1st_3rd_byte_neon((unsigned int *)rowptr, (unsigned int *)jpeg_out, iOutputSize/4);
    csc_swap_1st_3rd_byte_mask_neon((unsigned int *)rowptr, (unsigned int *)jpeg_out, iOutputSize/4, 0xff);

    m_bHwJpegFlag = false;
}

