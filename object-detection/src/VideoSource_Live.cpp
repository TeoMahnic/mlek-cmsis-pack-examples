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
#include "BufAttributes.hpp"

#include "image_processing_func.h"
#include "cmsis_vstream.h"
#include "cmsis_os2.h"

#include "log_macros.h"

/* Define input image bit depth */
#ifndef IMAGE_BIT_DEPTH
#define IMAGE_BIT_DEPTH         24
#endif

/* Define number of bytes per pixel */
#define IMAGE_COLOR_BYTES      (IMAGE_BIT_DEPTH / 8U)

/* Frame type */
#define CAMERA_FRAME_TYPE_RAW   0U
#define CAMERA_FRAME_TYPE_RGB   1U

/* Define camera RAW frame size */
#if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW)
#define CAMERA_FRAME_SIZE      (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT)
#else
#define CAMERA_FRAME_SIZE      (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * IMAGE_COLOR_BYTES)
#endif

/* Define RGB image size */
#define RGB_IMAGE_SIZE         (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * IMAGE_COLOR_BYTES)

/* Define ML image size */
#define ML_IMAGE_SIZE          (ML_IMAGE_WIDTH * ML_IMAGE_HEIGHT * IMAGE_COLOR_BYTES)

/* Define display image size */
#define DISPLAY_IMAGE_SIZE     (DISPLAY_FRAME_WIDTH * DISPLAY_FRAME_HEIGHT * IMAGE_COLOR_BYTES)


/* Reference to the underlying CMSIS vStream drivers */
extern vStreamDriver_t          Driver_vStreamVideoIn;
#define vStream_VideoIn       (&Driver_vStreamVideoIn)

extern vStreamDriver_t          Driver_vStreamVideoOut;
#define vStream_VideoOut      (&Driver_vStreamVideoOut)

/* Draws a box with the specified coordinates */
static void DrawBox(uint8_t *imageData, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h);

/* Camera frame buffer */
static uint8_t CAM_Frame[CAMERA_FRAME_SIZE] CAMERA_FRAME_BUF_ATTRIBUTE;

#if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW)
/* RGB image buffer */
static uint8_t RGB_Image[RGB_IMAGE_SIZE] RGB_IMAGE_BUF_ATTRIBUTE;
#endif

/* ML image buffer */
static uint8_t ML_Image[ML_IMAGE_SIZE] ML_IMAGE_BUF_ATTRIBUTE;

/* Display frame buffer */
static uint8_t LCD_Frame[DISPLAY_IMAGE_SIZE] DISPLAY_FRAME_BUF_ATTRIBUTE;

osThreadId_t tid_app_main = NULL;

uint8_t VideoIn_Ready = 0U;
uint8_t VideoOut_Ready = 0U;

uint32_t EventCnt_VideoIn = 0U;
uint32_t EventCnt_VideoOut = 0U;

/* Video In Stream Event Callback */
void VideoIn_Event_Callback (uint32_t event) {
    (void)event;

    if (event & VSTREAM_EVENT_DATA) {
        /* Video frame is available in camera frame buffer */
        osThreadFlagsSet(tid_app_main, 0x1);
    }
    EventCnt_VideoIn++;
}

/* Video Out Stream Event Callback */
void VideoOut_Event_Callback (uint32_t event) {
    (void)event;

    EventCnt_VideoOut++;
}

bool open_img_source(const uint32_t idx)
{
    uint8_t *inFrame;
    uint8_t *outFrame;
    vStreamStatus_t status;

    tid_app_main = osThreadGetId();

    if (VideoIn_Ready == 0U) {
        /* Initialize Video Input Stream */
        if (vStream_VideoIn->Initialize(VideoIn_Event_Callback) != VSTREAM_OK) {
            printf_err("Failed to initialise video input driver\n");
            return false;
        }

        /* Set Input Video buffer */
        if (vStream_VideoIn->SetBuf(CAM_Frame, sizeof(CAM_Frame), CAMERA_FRAME_SIZE) != VSTREAM_OK) {
            printf_err("Failed to set buffer for video input\n");
            return false;
        }

        VideoIn_Ready = 1U;
    }

    /* Start video capture */
    if (vStream_VideoIn->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
        printf_err("Failed to start video capture\n");
        return false;
    }


    /* Wait for new video input frame */
    osThreadFlagsWait(0x1, osFlagsWaitAny, osWaitForever);

    /* Get input video frame buffer */
    inFrame = (uint8_t *)vStream_VideoIn->GetBlock();
    if (inFrame == NULL) {
        printf_err("Failed to get video input frame\n");
        return false;
    }

    #if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW)
        /* Perform debayering */
        image_debayer(inFrame,
                    RGB_Image,
                    CAMERA_FRAME_WIDTH,
                    CAMERA_FRAME_HEIGHT,
                    CAMERA_FRAME_BAYER,
                    1U);
        /* Use RGB image for further processing */
        inFrame = RGB_Image;
    #endif

    /* Release input frame */
    if (vStream_VideoIn->ReleaseBlock() != VSTREAM_OK) {
        printf_err("Failed to release video input frame\n");
    }

    /* Resize image to fit ML model expected size */
    image_resize(inFrame,
                CAMERA_FRAME_WIDTH,
                CAMERA_FRAME_HEIGHT,
                (uint8_t *)ML_Image,
                ML_IMAGE_WIDTH,
                ML_IMAGE_HEIGHT);

    return true;
}

void close_img_source(const uint32_t idx)
{
    vStreamStatus_t status;
    uint8_t *outFrame;

    if (VideoOut_Ready == 0U) {
        /* Initialize Video Output Stream */
        if (vStream_VideoOut->Initialize(VideoOut_Event_Callback) != VSTREAM_OK) {
            printf_err("Failed to initialise video output driver\n");
            return;
        }

        /* Set Output Video buffer */
        if (vStream_VideoOut->SetBuf(LCD_Frame, sizeof(LCD_Frame), DISPLAY_IMAGE_SIZE) != VSTREAM_OK) {
            printf_err("Failed to set buffer for video output\n");
            return;
        }

        VideoOut_Ready = 1U;
    }

    /* Wait for video output frame to be released */
    do {
        status = vStream_VideoOut->GetStatus();
    } while (status.active == 1U);

    /* Get output frame */
    outFrame = (uint8_t *)vStream_VideoOut->GetBlock();
    if (outFrame == NULL) {
        printf_err("Failed to get video output frame\n");
        return;
    }

    /* Copy ML image into the display frame buffer */
    image_copy_to_framebuffer(ML_Image,
                              ML_IMAGE_WIDTH,
                              ML_IMAGE_HEIGHT,
                              outFrame,
                              DISPLAY_FRAME_WIDTH,
                              DISPLAY_FRAME_HEIGHT,
                             (DISPLAY_FRAME_WIDTH - ML_IMAGE_WIDTH) / 2,
                             (DISPLAY_FRAME_HEIGHT - ML_IMAGE_HEIGHT)/2);

    /* Release output frame */
    if (vStream_VideoOut->ReleaseBlock() != VSTREAM_OK) {
        printf_err("Failed to release video output frame\n");
    }

    /* Start video output */
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
    return ML_Image;
}

uint32_t get_img_array_size(const uint32_t idx)
{
    /* Return image array size in bytes */
    return sizeof(ML_Image);
}

void set_img_object_box(const uint32_t idx, const uint32_t x0, const uint32_t y0, const uint32_t w, const uint32_t h) {
    /* Draw a box around detected object */
    DrawBox(ML_Image, x0, y0, w, h);
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
    const uint32_t step = ML_IMAGE_WIDTH * 3;
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
