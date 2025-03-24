/*
 * SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <cstring>

#include "VideoSource.hpp" 
#include "video_drv.h"
#include "cmsis_os2.h"

#include "log_macros.h"

#define IMAGE_WIDTH     192
#define IMAGE_HEIGHT    192
#define IMAGE_SIZE      (IMAGE_WIDTH * IMAGE_HEIGHT * 3)

/* Draws a box with the specified coordinates */
static void DrawBox(uint8_t *imageData, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h);

/* RGB image buffer - cropped/scaled version of the original + debayered. */
static uint8_t inImage[IMAGE_SIZE];

/* LCD image buffer */
static uint8_t outImage[IMAGE_SIZE];

osThreadId_t tid_app_main = NULL;
osThreadId_t tid_video_capture = NULL;

void VideoDrv_Event_Callback (uint32_t channel, uint32_t event) {
    (void)channel;
    (void)event;

    osThreadFlagsSet(tid_video_capture, 0x0001);
}

/**
    Capture video frames and output them to the display
*/
void video_capture (void *arg) {
    VideoDrv_Status_t status;

    /* Initialize Video Interface */
    if (VideoDrv_Initialize(NULL) != VIDEO_DRV_OK) {
        printf_err("Failed to initialise video driver\n");
        return;
    }

    /* Configure Input Video */
    if (VideoDrv_Configure(VIDEO_DRV_IN0,  IMAGE_WIDTH, IMAGE_HEIGHT, VIDEO_DRV_COLOR_RGB888, 60U) != VIDEO_DRV_OK) {
        printf_err("Failed to configure video input\n");
        return;
    }
    /* Configure Output Video */
    if (VideoDrv_Configure(VIDEO_DRV_OUT0, IMAGE_WIDTH, IMAGE_HEIGHT, VIDEO_DRV_COLOR_RGB888, 60U) != VIDEO_DRV_OK) {
        printf_err("Failed to configure video output\n");
        return;
    }

    /* Set Input Video buffer */
    if (VideoDrv_SetBuf(VIDEO_DRV_IN0, inImage, IMAGE_SIZE) != VIDEO_DRV_OK) {
        printf_err("Failed to set buffer for video input\n");
        return;
    }
    /* Set Output Video buffer */
    if (VideoDrv_SetBuf(VIDEO_DRV_OUT0, outImage, IMAGE_SIZE) != VIDEO_DRV_OK) {
        printf_err("Failed to set buffer for video output\n");
        return;
    }

    /* Start video capture (single frame) */
    if (VideoDrv_StreamStart(VIDEO_DRV_IN0, VIDEO_DRV_MODE_SINGLE) != VIDEO_DRV_OK) {
        printf_err("Failed to start video capture\n");
        return;
    }

    void *inFrame;
    void *outFrame;

    while(1) {
        /* Wait for flag from video callback */
        osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);

        /* Get input video frame buffer */
        inFrame = VideoDrv_GetFrameBuf(VIDEO_DRV_IN0);

        /* Wait for video output frame to be released */
        do {
            status = VideoDrv_GetStatus(VIDEO_DRV_OUT0);
        } while (status.buf_full != 0U);

        /* Get output video frame buffer */
        outFrame = VideoDrv_GetFrameBuf(VIDEO_DRV_OUT0);

        /* Copy image frame */
        memcpy(outFrame, inFrame, IMAGE_SIZE);

        /* Release input frame */
        VideoDrv_ReleaseFrame(VIDEO_DRV_IN0);

        /* Start video capture (single frame) */
        if (VideoDrv_StreamStart(VIDEO_DRV_IN0, VIDEO_DRV_MODE_SINGLE) != VIDEO_DRV_OK) {
            printf_err("Failed to start video capture\n");
            return;
        }

        /* Buffer is ready, start processing it */
        osThreadFlagsSet(tid_app_main, 0x0001);
    }
}

bool open_img_source(const uint32_t idx)
{
    osThreadAttr_t const attr = {NULL, 0, NULL, 0, NULL, 0, osPriorityHigh, 0, 0};
    uint32_t flags;
    VideoDrv_Status_t status;

    if (tid_video_capture == NULL) {
        /* Get application thread ID */
        tid_app_main = osThreadGetId();

        /* Create video capture thread */
        tid_video_capture = osThreadNew(video_capture, NULL, &attr);
    }

    // #if POOLING
    /* Wait for video input frame */
    do {
        status = VideoDrv_GetStatus(VIDEO_DRV_IN0);
    } while (status.buf_empty != 0U);

    if (status.buf_empty == 0U) {
        osThreadFlagsSet(tid_video_capture, 0x0001);
    }
    // #endif

    /* Wait for flag from video capture thread (2 sec timeout) */
    flags = osThreadFlagsWait(0x0001, osFlagsWaitAny, 2000);

    if (flags == osFlagsErrorTimeout) {
        /* Capture thread did not set the event */
        return false;
    }

    return true;
}

void close_img_source(const uint32_t idx)
{
    /* Release output frame */
    VideoDrv_ReleaseFrame(VIDEO_DRV_OUT0);

    /* Start video output (single frame) */
    VideoDrv_StreamStart(VIDEO_DRV_OUT0, VIDEO_DRV_MODE_SINGLE);
}

const char* get_filename(const uint32_t idx)
{
    return "Live Video Stream";
}

const uint8_t* get_img_array(const uint32_t idx)
{
    return outImage;
}

uint32_t get_img_array_size(const uint32_t idx)
{
    /* Return image array size in bytes */
    return sizeof(outImage);
}

void set_img_object_box(const uint32_t idx, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h) {
    /* Draw a box around detected object */
    DrawBox(outImage, x0, y0, w, h);
}

/**
 * @brief Draws a box in the image.
 *
 * @param[out] imageData    Pointer to the start of the image.
 * @param[in]  width        Image width.
 * @param[in]  height       Image height.
 * @param[in]  result       Object detection result.
 */
static void DrawBox(uint8_t *imageData, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h)
{
    const uint32_t step = IMAGE_WIDTH * 3;
    uint8_t* const imStart = imageData + (y0 * step) + (x0 * 3);

    uint8_t* dst_0 = imStart;
    uint8_t* dst_1 = imStart + (h * step);

    for (uint32_t i = 0; i < w; ++i) {
        dst_0[1] = 255;
        dst_1[1] = 255;

        dst_0 += 3;
        dst_1 += 3;
    }

    dst_0 = imStart;
    dst_1 = imStart + (w * 3);

    for (uint32_t j = 0; j < h; ++j) {
        dst_0[1] = 255;
        dst_1[1] = 255;

        dst_0 += step;
        dst_1 += step;
    }
}
