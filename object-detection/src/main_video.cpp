/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 Arm Limited and/or its
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

/**
 * This object detection example is intended to work with the
 * CMSIS pack produced by ml-embedded-eval-kit. The pack consists
 * of platform agnostic end-to-end ML use case API's that can be
 * used to construct ML examples for any target that can support
 * the memory requirements for TensorFlow Lite Micro framework and
 * some heap for the API runtime.
 */
#include "BufAttributes.hpp" /* Buffer attributes to be applied */
#include "Classifier.hpp"    /* Classifier for the result */
#include "DetectionResult.hpp"
#include "DetectorPostProcessing.hpp" /* Post Process */
#include "DetectorPreProcessing.hpp"  /* Pre Process */
#include "YoloFastestModel.hpp"       /* Model API */
#include "main_video.h"

/* Platform dependent files */
#include "RTE_Components.h"  /* Provides definition for CMSIS_device_header */
#include CMSIS_device_header /* Gives us IRQ num, base addresses. */
#include "log_macros.h"      /* Logging macros (optional) */
#include "video_drv.h"       /* Video Driver API */

#define IMAGE_WIDTH     192
#define IMAGE_HEIGHT    192
#define IMAGE_SIZE      (IMAGE_WIDTH * IMAGE_HEIGHT * 3)

namespace arm {
namespace app {
    /* Tensor arena buffer */
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

    /* RGB image buffer - cropped/scaled version of the original + debayered. */
    static uint8_t rgbImage[IMAGE_SIZE];

    /* LCD image buffer */
    static uint8_t lcdImage[IMAGE_SIZE];

    /* Optional getter function for the model pointer and its size. */
    namespace object_detection {
        extern uint8_t* GetModelPointer();
        extern size_t GetModelLen();
    } /* namespace object_detection */
} /* namespace app */
} /* namespace arm */

typedef arm::app::object_detection::DetectionResult OdResults;

/**
 * @brief Draws a boxes in the image using the object detection results vector.
 *
 * @param[out] rgbImage     Pointer to the start of the image.
 * @param[in]  width        Image width.
 * @param[in]  height       Image height.
 * @param[in]  results      Vector of object detection results.
 */
static void DrawDetectionBoxes(uint8_t* rgbImage,
                               const uint32_t imageWidth,
                               const uint32_t imageHeight,
                               const std::vector<OdResults>& results);

int app_main()
{
    /* Model object creation and initialisation. */
    arm::app::YoloFastestModel model;
    if (!model.Init(arm::app::tensorArena,
                    sizeof(arm::app::tensorArena),
                    arm::app::object_detection::GetModelPointer(),
                    arm::app::object_detection::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return 1;
    }

    auto initialImgIdx = 0;

    TfLiteTensor* inputTensor   = model.GetInputTensor(0);
    TfLiteTensor* outputTensor0 = model.GetOutputTensor(0);
    TfLiteTensor* outputTensor1 = model.GetOutputTensor(1);

    if (!inputTensor->dims) {
        printf_err("Invalid input tensor dims\n");
        return 1;
    } else if (inputTensor->dims->size < 3) {
        printf_err("Input tensor dimension should be >= 3\n");
        return 1;
    }

    TfLiteIntArray* inputShape = model.GetInputShape(0);

    const int inputImgCols = inputShape->data[arm::app::YoloFastestModel::ms_inputColsIdx];
    const int inputImgRows = inputShape->data[arm::app::YoloFastestModel::ms_inputRowsIdx];

    /* Set up pre and post-processing. */
    arm::app::DetectorPreProcess preProcess =
        arm::app::DetectorPreProcess(inputTensor, true, model.IsDataSigned());

    std::vector<OdResults> results;
    const arm::app::object_detection::PostProcessParams postProcessParams{
        inputImgRows,
        inputImgCols,
        arm::app::object_detection::originalImageSize,
        arm::app::object_detection::anchor1,
        arm::app::object_detection::anchor2};
    arm::app::DetectorPostProcess postProcess =
        arm::app::DetectorPostProcess(outputTensor0, outputTensor1, results, postProcessParams);

    const size_t imgSz = inputTensor->bytes < IMAGE_SIZE ?
                         inputTensor->bytes : IMAGE_SIZE;

    if (sizeof(arm::app::lcdImage) < imgSz) {
        printf_err("RGB buffer is insufficient\n");
        return 3;
    }

    /* Initialize Video Interface */
    if (VideoDrv_Initialize(NULL) != VIDEO_DRV_OK) {
        printf_err("Failed to initialise video driver\n");
        return 1;
    }

    /* Configure Input Video */
    if (VideoDrv_Configure(VIDEO_DRV_IN0,  IMAGE_WIDTH, IMAGE_HEIGHT, VIDEO_DRV_COLOR_RGB888, 60U) != VIDEO_DRV_OK) {
        printf_err("Failed to configure video input\n");
        return 1;
    }
    /* Configure Output Video */
    if (VideoDrv_Configure(VIDEO_DRV_OUT0, IMAGE_WIDTH, IMAGE_HEIGHT, VIDEO_DRV_COLOR_RGB888, 60U) != VIDEO_DRV_OK) {
        printf_err("Failed to configure video output\n");
        return 1;
    }

    /* Set Input Video buffer */
    if (VideoDrv_SetBuf(VIDEO_DRV_IN0,  arm::app::rgbImage, IMAGE_SIZE) != VIDEO_DRV_OK) {
        printf_err("Failed to set buffer for video input\n");
        return 1;
    }
    /* Set Output Video buffer */
    if (VideoDrv_SetBuf(VIDEO_DRV_OUT0, arm::app::lcdImage, IMAGE_SIZE) != VIDEO_DRV_OK) {
        printf_err("Failed to set buffer for video output\n");
        return 1;
    }

    /* Start video capture (single frame) */
    if (VideoDrv_StreamStart(VIDEO_DRV_IN0, VIDEO_DRV_MODE_SINGLE) != VIDEO_DRV_OK) {
        printf_err("Failed to start video capture\n");
        return 1;
    }

    auto dstPtr = static_cast<uint8_t*>(inputTensor->data.uint8);

    uint32_t imgCount = 0;

    void *rgbFrame;
    void *lcdFrame;

    while (true) {
        VideoDrv_Status_t status;

        results.clear();

        /* Wait for video input frame */
        do {
            status = VideoDrv_GetStatus(VIDEO_DRV_IN0);
        } while (status.buf_empty != 0U);

        /* Get input video frame buffer */
        rgbFrame = VideoDrv_GetFrameBuf(VIDEO_DRV_IN0);
        /* Get output video frame buffer */
        lcdFrame = VideoDrv_GetFrameBuf(VIDEO_DRV_OUT0);

        /* Copy image frame */
        memcpy(lcdFrame, rgbFrame, IMAGE_SIZE);

        /* Release input frame */
        VideoDrv_ReleaseFrame(VIDEO_DRV_IN0);

        /* Start video capture (single frame) */
        if (VideoDrv_StreamStart(VIDEO_DRV_IN0, VIDEO_DRV_MODE_SINGLE) != VIDEO_DRV_OK) {
            printf_err("Failed to start video capture\n");
            return 1;
        }

        /* Run the pre-processing, inference and post-processing. */
        if (!preProcess.DoPreProcess(lcdFrame, imgSz)) {
            printf_err("Pre-processing failed.\n");
            return 1;
        }

        /* Run inference over this image. */
        if (!(imgCount++ & 0xF)) {
            printf("\rImage %" PRIu32 "; ", imgCount);
        }

        if (!model.RunInference()) {
            printf_err("Inference failed.\n");
            return 2;
        }

        if (!postProcess.DoPostProcess()) {
            printf_err("Post-processing failed.\n");
            return 3;
        }

        /* Draw detection boxes to output frame buffer */
        DrawDetectionBoxes((uint8_t *)lcdFrame, inputImgCols, inputImgRows, results);

        /* Release output frame */
        VideoDrv_ReleaseFrame(VIDEO_DRV_OUT0);

        /* Start video output (single frame) */
        VideoDrv_StreamStart(VIDEO_DRV_OUT0, VIDEO_DRV_MODE_SINGLE);
    }

    return 0;
}

/**
 * @brief Draws a box in the image using the object detection result object.
 *
 * @param[out] imageData    Pointer to the start of the image.
 * @param[in]  width        Image width.
 * @param[in]  height       Image height.
 * @param[in]  result       Object detection result.
 */
static void DrawBox(uint8_t* imageData,
                    const uint32_t width,
                    const uint32_t height,
                    const OdResults& result)
{
    const auto x = result.m_x0;
    const auto y = result.m_y0;
    const auto w = result.m_w;
    const auto h = result.m_h;

    const uint32_t step = width * 3;
    uint8_t* const imStart = imageData + (y * step) + (x * 3);

    uint8_t* dst_0 = imStart;
    uint8_t* dst_1 = imStart + (h * step);

    for (uint32_t i = 0; i < w; ++i) {
        *dst_0 = 255;
        *dst_1 = 255;

        dst_0 += 3;
        dst_1 += 3;
    }

    dst_0 = imStart;
    dst_1 = imStart + (w * 3);

    for (uint32_t j = 0; j < h; ++j) {
        *dst_0 = 255;
        *dst_1 = 255;

        dst_0 += step;
        dst_1 += step;
    }
}

static void DrawDetectionBoxes(uint8_t* rgbImage,
                               const uint32_t imageWidth,
                               const uint32_t imageHeight,
                               const std::vector<OdResults>& results)
{
    for (const auto& result : results) {
        DrawBox(rgbImage, imageWidth, imageHeight, result);
        printf("Detection :: [%" PRIu32 ", %" PRIu32
                         ", %" PRIu32 ", %" PRIu32 "]\n",
                result.m_x0,
                result.m_y0,
                result.m_w,
                result.m_h);
    }
}
