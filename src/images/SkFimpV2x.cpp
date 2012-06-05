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

#include "SkFimpV2x.h"
#include "csc_swap_neon.h"

int SecJPEGCodec::m_setCache()
{
    int ret = 0;

    if (m_jpeg_fd < 0) {
        LOGE("[%s] JPEG open failed:%d",__func__, m_jpeg_fd);
        return -1;
    }

    ret = jpeghal_s_ctrl(m_jpeg_fd, V4L2_CID_CACHEABLE, 1);

    return ret;
}

int SecJPEGCodec::m_createDecoder()
{
    int ret = 0;

    m_jpeg_mode = JPEG_DECODE;

    m_jpeg_fd = jpeghal_dec_init();
    if (m_jpeg_fd <= 0)
        ret = -1;

    ret = m_setCache();

    return ret;
}

int SecJPEGCodec::m_destroyDecoder()
{
    int ret = 0;

    if (m_jpeg_fd < 0)
        close(m_jpeg_fd);
    else
        ret = jpeghal_deinit(m_jpeg_fd, &m_in_buf, &m_out_buf);

    return ret;
}

int SecJPEGCodec::m_createEncoder()
{
    int ret = 0;

    m_jpeg_mode = JPEG_ENCODE;

    m_jpeg_fd = jpeghal_enc_init();
    if (m_jpeg_fd <= 0)
        ret = -1;

    m_setCache();

    return ret;
}

int SecJPEGCodec::m_destroyEncoder()
{
    int ret = 0;

    if (m_jpeg_fd < 0)
        close(m_jpeg_fd);
    else
        ret = jpeghal_deinit(m_jpeg_fd, &m_in_buf, &m_out_buf);

    return ret;
}

#ifdef SEC_HWJPEG_G2D
int SecJPEGCodec::executeG2D(SkBitmap *bm, int in_width, int in_height, void *srcaddr, void *dstaddr)
{
    int ret = 0;

    m_fimg.srcX = 0;
    m_fimg.srcY = 0;
    m_fimg.srcW = in_width;
    m_fimg.srcH = in_height;
    m_fimg.srcFWStride = in_height * 4;
    m_fimg.srcFH = in_height;
    m_fimg.srcBPP = 4;
    m_fimg.srcColorFormat = bm->getConfig();
    m_fimg.srcOrder = 2; // AX_BGR
    m_fimg.srcAddr = (unsigned char *)srcaddr;

    m_fimg.isSolidFill = false;

    m_fimg.dstX = 0;
    m_fimg.dstY = 0;
    m_fimg.dstW = bm->width();
    m_fimg.dstH = bm->height();
    m_fimg.dstFWStride = bm->rowBytes();
    m_fimg.dstFH = bm->height();
    m_fimg.dstBPP = bm->bytesPerPixel();
    m_fimg.dstColorFormat = bm->getConfig();
    m_fimg.dstOrder = 0; // AX_RGB
    m_fimg.dstAddr = (unsigned char *)dstaddr;

    m_fimg.clipT = 0;
    m_fimg.clipB = m_fimg.dstW;
    m_fimg.clipL = 0;
    m_fimg.clipR = m_fimg.dstH;

    m_fimg.mskAddr = NULL;

    m_fimg.fillcolor = 0;
    m_fimg.rotate = 0;
    m_fimg.alpha = 255;
    m_fimg.xfermode = kSrc_Mode;
    m_fimg.isDither = false;
    m_fimg.isFilter = 0;
    m_fimg.colorfilter = 0;
    m_fimg.canusehybrid = 0;
    m_fimg.matrixType = 0;
    m_fimg.matrixSx = 0;
    m_fimg.matrixSy = 0;

    FimgApiStretch(&m_fimg, __func__);

    return ret;
}
#endif

SecJPEGCodec::SecJPEGCodec() :
    m_jpeg_fd       (-1),
    m_numplanes     (1),
    m_hwjpeg_flag   (false)
{
    memset(&m_in_buf, 0, sizeof(struct jpeg_buf));
    memset(&m_out_buf, 0, sizeof(struct jpeg_buf));
    memset(&m_jpeg_cfg, 0, sizeof(struct jpeg_config));
#ifdef SEC_HWJPEG_G2D
    memset(&m_fimg, 0, sizeof(struct Fimg));
#endif
}

SecJPEGCodec::~SecJPEGCodec()
{
    if (m_jpeg_mode == JPEG_DECODE)
        m_destroyDecoder();
    else if (m_jpeg_mode == JPEG_ENCODE)
        m_destroyEncoder();
}

int SecJPEGCodec::checkHwJPEGSupport(SkBitmap::Config config, jpeg_decompress_struct *cinfo, int sampleSize)
{
    int ret = 0;
    int scalingratio = 1;

    if (sampleSize == 1) {
        if ((cinfo->image_width % 2 == 0) && (cinfo->image_height % 2 == 0)) {
            m_hwjpeg_flag = true;
        } else {
            m_hwjpeg_flag = false;
        }
    } else if (sampleSize >= 4) {
        if (((cinfo->image_width / 4) % 2 == 0) && ((cinfo->image_height / 4) % 2 == 0)) {
            m_hwjpeg_flag = true;
            scalingratio = 4;
        } else {
            m_hwjpeg_flag = false;
        }
    } else if (sampleSize >= 2) {
        if (((cinfo->image_width / 2) % 2 == 0) && ((cinfo->image_height / 2) % 2 == 0)) {
            m_hwjpeg_flag = true;
            scalingratio = 2;
        } else {
            m_hwjpeg_flag = false;
        }
    } else {
        m_hwjpeg_flag = false;
        LOGE("[SKIA JPEG] HW JPEG not supported scaling");
    }

    if ((m_hwjpeg_flag == true) &&
        (cinfo->progressive_mode == false) &&
        ((config == SkBitmap::kARGB_8888_Config) ||
        (config == SkBitmap::kRGB_565_Config))) {
        m_hwjpeg_flag = true;
        cinfo->output_width = cinfo->image_width / scalingratio;
        cinfo->output_height = cinfo->image_height / scalingratio;
    } else {
            m_hwjpeg_flag = false;
    }

    return m_hwjpeg_flag;
}

int SecJPEGCodec::setDecodeHwJPEGConfig(SkBitmap::Config config, jpeg_decompress_struct *cinfo, SkStream *stream)
{
    int out_fmt = V4L2_PIX_FMT_RGB32;
    int jpegsize;
    int ret = 0;

    ret = m_createDecoder();
    if (ret < 0) {
        LOGE("[%s]JPEG Decode Init failed", __func__);
        return -1;
    }

    switch (config) {
        case SkBitmap::kARGB_8888_Config:
            out_fmt = V4L2_PIX_FMT_RGB32;
            break;
        case SkBitmap::kRGB_565_Config:
            out_fmt = V4L2_PIX_FMT_RGB565X;
            LOGD("[%s]JPEG Decode format is RGB565", __func__);
            break;
        default:
            return -1;
            LOGE("[%s]JPEG Decode Skia config failed", __func__);
    }

    jpegsize = stream->getLength();

    if (jpegsize <= 0) {
        LOGE("[%s] in_size is too small", __func__);
        return -1;
    }

    m_jpeg_cfg.mode = m_jpeg_mode;
    m_jpeg_cfg.width = cinfo->image_width;
    m_jpeg_cfg.height = cinfo->image_height;
    m_jpeg_cfg.scaled_width = cinfo->output_width;
    m_jpeg_cfg.scaled_height = cinfo->output_height;
    m_jpeg_cfg.sizeJpeg = jpegsize;
    m_jpeg_cfg.num_planes = 1;
    m_jpeg_cfg.pix.dec_fmt.in_fmt = V4L2_PIX_FMT_JPEG_422;
    m_jpeg_cfg.pix.dec_fmt.out_fmt = out_fmt;

    ret = jpeghal_dec_setconfig(m_jpeg_fd, &m_jpeg_cfg);

    return ret;
}

int SecJPEGCodec::setEncodeHwJPEGConfig(int in_fmt, int out_fmt, int width, int height, int qual)
{
    int ret = 0;

    m_createEncoder();

    m_jpeg_cfg.mode = m_jpeg_mode;
    m_jpeg_cfg.enc_qual = (enum jpeg_quality_level)qual;
    m_jpeg_cfg.width = width;
    m_jpeg_cfg.height = height;
    m_jpeg_cfg.num_planes = 1;
    m_jpeg_cfg.pix.dec_fmt.in_fmt = in_fmt;
    m_jpeg_cfg.pix.dec_fmt.out_fmt = out_fmt;

    ret = jpeghal_enc_setconfig(m_jpeg_fd, &m_jpeg_cfg);

    return ret;
}

int SecJPEGCodec::setHwJPEGInputBuffer(SkStream *stream)
{
    const void *stream_data;
    const void *eoi_addr;
    int eoi_marker;
    int ret = 0;

    m_in_buf.memory = V4L2_MEMORY_MMAP;
    m_in_buf.num_planes = 1;
    ret = jpeghal_set_inbuf(m_jpeg_fd, &m_in_buf);

    stream_data = stream->getMemoryBase();

    if (stream_data == NULL) {
        if (!stream->rewind()) {
            LOGD("[SKIA JPEG] rewind() is failed");
            return -1;
        }
        size_t bytes = stream->read(m_in_buf.start[0], m_in_buf.length[0]);
        eoi_addr = m_in_buf.start[0] + m_in_buf.length[0] - 4;
        eoi_marker = (int)(*(int *)eoi_addr);
        if ((eoi_marker & 0xffff0000) != (int)0xd9ff0000) {
            LOGD("[SKIA JPEG] EOI is missed");
            return -1;
        }
    } else {
        memcpy(m_in_buf.start[0], stream_data, m_in_buf.length[0]);
    }

    return ret;
}

const void *SecJPEGCodec::setHwJPEGOutputBuffer(void)
{
    int ret = 0;

    m_out_buf.memory = V4L2_MEMORY_MMAP;
    m_out_buf.num_planes = 1;
    ret = jpeghal_set_outbuf(m_jpeg_fd, &m_out_buf);

    return (const void *)(m_out_buf.start[0]);
}

int SecJPEGCodec::executeDecodeHwJPEG(void)
{
    int ret = 0;

    ret = jpeghal_dec_exe(m_jpeg_fd, &m_in_buf, &m_out_buf);

    return ret;
}

int SecJPEGCodec::executeEncodeHwJPEG(void)
{
    int ret = 0;

    ret = jpeghal_enc_exe(m_jpeg_fd, &m_in_buf, &m_out_buf);

    return ret;
}

void SecJPEGCodec::outputJpeg(SkBitmap *bm, unsigned char *rowptr)
{
    const void *jpeg_out;

    jpeg_out = (const void *)m_out_buf.start[0];

#ifdef SEC_HWJPEG_G2D
    executeG2D(bm, cinfo.image_width, cinfo.image_height, jpeg_out, rowptr);
#else
    csc_swap_1st_3rd_byte_mask_neon((unsigned int *)rowptr, (unsigned int *)jpeg_out, m_out_buf.length[0]/4, 0xFF);
#endif
    m_hwjpeg_flag = false;
}
