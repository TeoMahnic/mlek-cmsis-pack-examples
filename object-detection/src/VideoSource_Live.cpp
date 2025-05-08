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
#include "cmsis_vstream.h"
#include "cmsis_os2.h"

#include "log_macros.h"

#define IMAGE_WIDTH     192
#define IMAGE_HEIGHT    192
#define IMAGE_SIZE      (IMAGE_WIDTH * IMAGE_HEIGHT * 3)

/* Reference to the underlying CMSIS vStream drivers */
extern vStreamDriver_t          Driver_vStreamVideoIn;
#define vStream_VideoIn       (&Driver_vStreamVideoIn)

extern vStreamDriver_t          Driver_vStreamVideoOut;
#define vStream_VideoOut      (&Driver_vStreamVideoOut)

/* Draws a box with the specified coordinates */
static void DrawBox(uint8_t *imageData, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h);

/* RGB image buffer - cropped/scaled version of the original + debayered. */
static uint8_t inImage[IMAGE_SIZE];

/* LCD image buffer */
static uint8_t outImage[IMAGE_SIZE];

osThreadId_t tid_app_main = NULL;
osThreadId_t tid_video_capture = NULL;

/* Video In Stream Event Callback */
void VideoIn_Event_Callback (uint32_t event) {
    (void)event;

    osThreadFlagsSet(tid_video_capture, 0x0001);
}

/* Video Out Stream Event Callback */
void VideoOut_Event_Callback (uint32_t event) {
    (void)event;
}

/**
    Capture video frames and output them to the display
*/
void video_capture (void *arg) {
    vStreamStatus_t status;
    void *inFrame;
    void *outFrame;

    /* Initialize Video Interface */
    if (vStream_VideoIn->Initialize(VideoIn_Event_Callback) != VSTREAM_OK) {
        printf_err("Failed to initialise video input driver\n");
        return;
    }
    if (vStream_VideoOut->Initialize(VideoOut_Event_Callback) != VSTREAM_OK) {
        printf_err("Failed to initialise video output driver\n");
        return;
    }

    /* Set Input Video buffer */
    if (vStream_VideoIn->SetBuf(inImage, sizeof(inImage), IMAGE_SIZE) != VSTREAM_OK) {
        printf_err("Failed to set buffer for video input\n");
        return;
    }
    /* Set Output Video buffer */
    if (vStream_VideoOut->SetBuf(outImage, sizeof(outImage), IMAGE_SIZE) != VSTREAM_OK) {
        printf_err("Failed to set buffer for video output\n");
        return;
    }

    /* Start video capture (single frame) */
    if (vStream_VideoIn->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
        printf_err("Failed to start video capture\n");
        return;
    }

    while(1) {
        /* Wait for flag from video callback */
        osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);

        /* Get input video frame buffer */
        inFrame = vStream_VideoIn->GetBlock();

        /* Wait for video output frame to be released */
        do {
            status = vStream_VideoOut->GetStatus();
        } while (status.active == 1U);

        /* Get output video frame buffer */
        outFrame = vStream_VideoOut->GetBlock();

        /* Copy image frame */
        memcpy(outFrame, inFrame, IMAGE_SIZE);

        /* Release input frame */
        if (vStream_VideoIn->ReleaseBlock() != VSTREAM_OK) {
            printf_err("Failed to release video input frame\n");
        }

        /* Start video capture (single frame) */
        if (vStream_VideoIn->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
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
    vStreamStatus_t status;

    if (tid_video_capture == NULL) {
        /* Get application thread ID */
        tid_app_main = osThreadGetId();

        /* Create video capture thread */
        tid_video_capture = osThreadNew(video_capture, NULL, &attr);
    }

    /* Wait for video input frame */
    do {
        status = vStream_VideoIn->GetStatus();
    } while (status.active == 1U);

    if (status.active == 0U) {
        osThreadFlagsSet(tid_video_capture, 0x0001);
    }

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
    if (vStream_VideoOut->ReleaseBlock() != VSTREAM_OK) {
        printf_err("Failed to release video output frame\n");
    }

    /* Start video output (single frame) */
    if (vStream_VideoOut->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
        printf_err("Failed to start video output\n");
    }
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
