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

#ifndef __SEC_JPEG_DECODE_FOR_SKIA_H__
#define __SEC_JPEG_DECODE_FOR_SKIA_H__

#include "SkImageDecoder.h"
#include "SkJpegUtility.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkUtils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
    #include <cutils/log.h>
    #include <SecJpegCodecHal.h>
}

#define JPEG_HW_MIN_WIDTH 256
#define JPEG_HW_MIN_HEIGHT 256
#define JPEG_WA_BUFFER_SIZE 64

class SecJpegDecoderForSkia {
public:
    ;
    enum ERROR_JPEG_DEC_SKIA{
        ERROR_ALREADY_CREATE = -0x300,
        ERROR_CANNOT_CREATE_SEC_JPEG_DEC_HAL,
        ERROR_ALREADY_DESTROY,
        ERROR_GET_IN_BUF_FAIL,
        ERROR_SET_IN_BUF_FAIL,
        ERROR_SET_OUT_BUF_FAIL,
        ERROR_REWIND_FAIL,
        ERROR_HW_CANNOT_SUPPORT,
        ERROR_JPEG_DEVICE_ALREADY_CREATE = -0x100,
        ERROR_CANNOT_OPEN_JPEG_DEVICE,
        ERROR_JPEG_DEVICE_ALREADY_CLOSED,
        ERROR_JPEG_DEVICE_ALREADY_DESTROY,
        ERROR_JPEG_DEVICE_NOT_CREATE_YET,
        ERROR_INVALID_COLOR_FORMAT,
        ERROR_INVALID_JPEG_FORMAT,
        ERROR_JPEG_CONFIG_POINTER_NULL,
        ERROR_INVALID_JPEG_CONFIG,
        ERROR_IN_BUFFER_CREATE_FAIL,
        ERROR_OUT_BUFFER_CREATE_FAIL,
        ERROR_EXCUTE_FAIL,
        ERROR_JPEG_SIZE_TOO_SMALL,
        ERROR_CANNOT_CHANGE_CACHE_SETTING,
        ERROR_SIZE_NOT_SET_YET,
        ERROR_BUFFR_IS_NULL,
        ERROR_BUFFER_TOO_SMALL,
        ERROR_GET_SIZE_FAIL,
        ERROR_REQBUF_FAIL,
        ERROR_INVALID_V4l2_BUF_TYPE = -0x80,
        ERROR_MMAP_FAILED,
        ERROR_FAIL,
        ERROR_NONE = 0
    };

    bool       checkHwJPEGSupport(SkBitmap::Config config, jpeg_decompress_struct *cinfo, int sampleSize);
    bool       isHwJPEG() { return m_bHwJpegFlag; }
    void       setHwJPEGFlag(int flag) { m_bHwJpegFlag= flag; }
    int        setDecodeHwJPEGConfig(SkBitmap::Config config, jpeg_decompress_struct *cinfo, SkStream *stream);

    int        setHwJPEGInputBuffer(char *pcBuffer, int iSize);
    int        setHwJPEGOutputBuffer(char *pcBuffer, int iSize);

    int         executeDecodeHwJPEG(void);
    void        outputJpeg(SkBitmap *bm, unsigned char *rowptr);
public:
    SecJpegDecoderForSkia();
    ~SecJpegDecoderForSkia();
private:
    int create(void);
    int destroy(void);


private:
    bool              m_bHwJpegFlag;
    SecJpegDecoderHal *m_pcJpegDec;
    char *m_pcJpegInBuf;
    char *m_pcJpegOutBuf;
    int m_iOutImageSize;
    int m_iInBufSize;
    int m_iOutBufSize;
    char *m_pcTmpInBuf;
    char *m_pcTmpOutBuf;
};

#endif //__SEC_JPEG_DECODE_FOR_SKIA_H__
