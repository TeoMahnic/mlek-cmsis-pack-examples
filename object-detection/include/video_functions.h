/*
 * SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its
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
#ifndef VIDEO_FUNCTIONS_HPP
#define VIDEO_FUNCTIONS_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum ColourFilter {
    BGGR,
    GBRG,
    GRBG,
    RGGB,
    Invalid
};

/**
 * @brief Get a cropped, colour corrected RGB frame from a RAW frame.
 *
 * @param[in] rawImgData        Pointer to the source (RAW) image.
 * @param[in] rawImgWidth       Width of the source image.
 * @param[in] rawImgHeight      Height of the source image.
 * @param[in] rawImgCropOffsetX Offset for X-axis from the source image (crop starts here).
 * @param[in] rawImgCropOffsetY Offset for Y-axis from the source image (crop starts here).
 * @param[out] rgbImgData       Pointer to the destination image (RGB) buffer.
 * @param[in] rgbImgWidth       Width of destination image.
 * @param[in] rgbImgHeight      Height of destination image.
 * @param[in] bayerFormat       Bayer format description code.
 * @return int                  0 if successful, 1 otherwise.
 */
int CropAndDebayer(
    const uint8_t* rawImgData,
    uint32_t rawImgWidth,
    uint32_t rawImgHeight,
    uint32_t rawImgCropOffsetX,
    uint32_t rawImgCropOffsetY,
    uint8_t* rgbImgData,
    uint32_t rgbImgWidth,
    uint32_t rgbImgHeight,
    enum ColourFilter bayerFormat);

#ifdef __cplusplus
}
#endif

#endif /* VIDEO_FUNCTIONS_HPP */
