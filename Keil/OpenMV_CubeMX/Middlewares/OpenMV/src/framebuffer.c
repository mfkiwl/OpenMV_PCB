/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Framebuffer stuff.
 *
 */
#include "imlib.h"
#include "omv_boardconfig.h"
#include "framebuffer.h"

//extern char _fb_base;
char sys_buf[OMV_FB_SIZE + OMV_FB_ALLOC_SIZE] __attribute__((at(OMV_AXI_SRAM_ORIGIN))) = {0};
framebuffer_t _fb_base = {0,0,0,0,0,0,0,(uint8_t *)sys_buf};

framebuffer_t *fb_framebuffer = (framebuffer_t *) &_fb_base;

//extern char _jpeg_buf;
char _jpeg_buf[OMV_JPEG_BUF_SIZE]__attribute__((at(OMV_SRAM3_ORIGIN))) = {0};
jpegbuffer_t *jpeg_fb_framebuffer = (jpegbuffer_t *) &_jpeg_buf;

uint32_t fb_buffer_size()
{
    switch (MAIN_FB()->bpp) {
        case IMAGE_BPP_BINARY: {
            return ((MAIN_FB()->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * MAIN_FB()->h;
        }
        case IMAGE_BPP_GRAYSCALE: {
            return (MAIN_FB()->w * MAIN_FB()->h) * sizeof(uint8_t);
        }
        case IMAGE_BPP_RGB565: {
            return (MAIN_FB()->w * MAIN_FB()->h) * sizeof(uint16_t);
        }
        case IMAGE_BPP_BAYER: {
            return MAIN_FB()->w * MAIN_FB()->h;
        }
        default: { // JPEG
            return MAIN_FB()->bpp;
        }
    }
}

void fb_update_jpeg_buffer()
{
    static int overflow_count = 0;

    if ((MAIN_FB()->bpp > 3) && JPEG_FB()->enabled) {
        // Lock FB
        if (mutex_try_lock(&JPEG_FB()->lock, MUTEX_TID_OMV)) {
            if((OMV_JPEG_BUF_SIZE-64) < MAIN_FB()->bpp) {
                // image won't fit. so don't copy.
                JPEG_FB()->w = 0; JPEG_FB()->h = 0; JPEG_FB()->size = 0;
            } else {
                memcpy(JPEG_FB()->pixels, MAIN_FB()->pixels, MAIN_FB()->bpp);
                JPEG_FB()->w = MAIN_FB()->w; JPEG_FB()->h = MAIN_FB()->h; JPEG_FB()->size = MAIN_FB()->bpp;
            }

            // Unlock the framebuffer mutex
            mutex_unlock(&JPEG_FB()->lock, MUTEX_TID_OMV);
        }
    } else if ((MAIN_FB()->bpp >= 0) && JPEG_FB()->enabled) {
        // Lock FB
        if (mutex_try_lock(&JPEG_FB()->lock, MUTEX_TID_OMV)) {
            // Set JPEG src and dst images.
            image_t src = {.w=MAIN_FB()->w, .h=MAIN_FB()->h, .bpp=MAIN_FB()->bpp,     .pixels=MAIN_FB()->pixels};
            image_t dst = {.w=MAIN_FB()->w, .h=MAIN_FB()->h, .bpp=(OMV_JPEG_BUF_SIZE-64),  .pixels=JPEG_FB()->pixels};

            // Note: lower quality saves USB bandwidth and results in a faster IDE FPS.
            bool overflow = false; //jpeg_compress(&src, &dst, JPEG_FB()->quality, false);
            DEBUG("jpeg_compress");
            if (overflow == true) {
                // JPEG buffer overflowed, reduce JPEG quality for the next frame
                // and skip the current frame. The IDE doesn't receive this frame.
                if (JPEG_FB()->quality > 1) {
                    // Keep this quality for the next n frames
                    overflow_count = 60;
                    JPEG_FB()->quality = IM_MAX(1, (JPEG_FB()->quality/2));
                }
                JPEG_FB()->w = 0; JPEG_FB()->h = 0; JPEG_FB()->size = 0;
            } else {
                if (overflow_count) {
                    overflow_count--;
                }
                // No buffer overflow, increase quality up to max quality based on frame size
                if (overflow_count == 0 && JPEG_FB()->quality
                       < ((fb_buffer_size() > JPEG_QUALITY_THRESH) ? JPEG_QUALITY_LOW:JPEG_QUALITY_HIGH)) {
                    JPEG_FB()->quality++;
                }
                // Set FB from JPEG image
                JPEG_FB()->w = dst.w; JPEG_FB()->h = dst.h; JPEG_FB()->size = dst.bpp;
            }

            // Unlock the framebuffer mutex
            mutex_unlock(&JPEG_FB()->lock, MUTEX_TID_OMV);
        }
    }
}
