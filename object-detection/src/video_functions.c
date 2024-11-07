/*
 * SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its
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
#include <stdint.h>
#include "video_functions.h"

/* Approximations for colour correction */
#define CLAMP_UINT8(x)          (x > 255 ? 255 : x < 0 ? 0 : x)
#define RED_WITH_CCM(r, g, b)   \
    CLAMP_UINT8(((int)r << 1) - ((int)g * 7) / 19 - (((int)b * 14) / 22))
#define GREEN_WITH_CCM(r, g, b) \
    CLAMP_UINT8(((int)g * 13 / 10) - ((int)r >> 1) + (((int)b * 6) / 37))
#define BLUE_WITH_CCM(r, g, b)  \
    CLAMP_UINT8(((int)b * 3) - (((int)r * 5) / 36) - (((int)g << 1) / 3))

typedef void (*DebayerRowPopulateFunction)(const uint8_t* pSrc,
                                           uint8_t* pDst,
                                            const uint32_t rawImgStep);

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a BGGR tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromBGGR(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t b = pSrc[0];
    int32_t g = (pSrc[1] + pSrc[rawImgStep]) >> 1;
    int32_t r = pSrc[rawImgStep + 1];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a GBRG tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromGBRG(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t g = (pSrc[0] + pSrc[rawImgStep + 1]) >> 1;
    int32_t b = pSrc[1];
    int32_t r = pSrc[rawImgStep];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a GRBG tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromGRBG(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t g = (pSrc[0] + pSrc[rawImgStep + 1]) >> 1;
    int32_t r = pSrc[1];
    int32_t b = pSrc[rawImgStep];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Populates the destination RGB pixel values from source expecting
 *          a RGGB tile pattern.
 * @param[in]   pSrc        Source pointer for raw image.
 * @param[out]  pDst        Starting address for the RGB image pixel to be
 *                          populated.
 * @param[in]   rawImgStep  Bytes to jump to the next row in the raw image.
 */
static inline void PopulateRGBFromRGGB(const uint8_t* pSrc,
                                       uint8_t* pDst,
                                       const uint32_t rawImgStep)
{
    int32_t r = pSrc[0];
    int32_t g = (pSrc[1] + pSrc[rawImgStep]) >> 1;
    int32_t b = pSrc[rawImgStep + 1];

    pDst[0] = RED_WITH_CCM(r,g,b);
    pDst[1] = GREEN_WITH_CCM(r,g,b);
    pDst[2] = BLUE_WITH_CCM(r,g,b);
}

/**
 * @brief   Gets the starting tile bayer tile pattern given the original bayer
 *          pattern and the offsets in the raw image.
 * @param[in]   format  Original RAW bayer format.
 * @param[in]   offsetX X-axis offset for the raw image.
 * @param[in]   offsetY Y-axis offset for the raw image.
 * @return      Tile pattern at the given offsets expressed as `ColourFilter`.
 */
static inline enum ColourFilter GetStartingTilePattern(
    const enum ColourFilter format,
    const uint32_t offsetX,
    const uint32_t offsetY)
{
    /** Get the indication for how many odd offsets are there and what's the pattern  */
    const uint32_t oddOffsetsScore = ((offsetX & 1) + ((offsetY & 1)<<1)) & 0x3 ;

    enum ColourFilter startingPattern = Invalid;

    switch (format) {
        case BGGR:
            switch (oddOffsetsScore) {
                case 0: startingPattern = BGGR; break;
                case 1: startingPattern = GBRG; break;
                case 2: startingPattern = GRBG; break;
                case 3: startingPattern = RGGB; break;
                default: startingPattern = Invalid;
            }
            break;

        case GBRG:
            switch (oddOffsetsScore) {
                case 0: startingPattern = GBRG; break;
                case 1: startingPattern = BGGR; break;
                case 2: startingPattern = RGGB; break;
                case 3: startingPattern = GRBG; break;
                default: startingPattern = Invalid;
            }
            break;
        break;

        case GRBG:
            switch (oddOffsetsScore) {
                case 0: startingPattern = GRBG; break;
                case 1: startingPattern = RGGB; break;
                case 2: startingPattern = BGGR; break;
                case 3: startingPattern = GBRG; break;
                default: startingPattern = Invalid;
            }
            break;
        break;

        case RGGB:
            switch (oddOffsetsScore) {
                case 0: startingPattern = RGGB; break;
                case 1: startingPattern = GRBG; break;
                case 2: startingPattern = GBRG; break;
                case 3: startingPattern = BGGR; break;
                default: startingPattern = Invalid;
            }
            break;

        default:
            startingPattern = Invalid;
    }

    return startingPattern;
}

/**
 * @brief Get the order in which debayering functions need to be called.
 * @param[in]   tilePattern             RAW image tile pattern at starting
 *                                      pixel.
 * @param[out]   deyaringFunctionArray  Array of functions to be populated.
 * @return 0 if successful, 1 otherwise.
 */
static int GetDebayeringFunctionOrder(
    const enum ColourFilter tilePattern,
    DebayerRowPopulateFunction deyaringFunctionArray[4])
{
    switch (tilePattern) {
        case BGGR:
            deyaringFunctionArray[0] = PopulateRGBFromBGGR;
            deyaringFunctionArray[1] = PopulateRGBFromGBRG;
            deyaringFunctionArray[2] = PopulateRGBFromGRBG;
            deyaringFunctionArray[3] = PopulateRGBFromRGGB;
            break;
        case GBRG:
            deyaringFunctionArray[0] = PopulateRGBFromGBRG;
            deyaringFunctionArray[1] = PopulateRGBFromBGGR;
            deyaringFunctionArray[2] = PopulateRGBFromRGGB;
            deyaringFunctionArray[3] = PopulateRGBFromGRBG;
            break;
        case GRBG:
            deyaringFunctionArray[0] = PopulateRGBFromGRBG;
            deyaringFunctionArray[1] = PopulateRGBFromRGGB;
            deyaringFunctionArray[2] = PopulateRGBFromBGGR;
            deyaringFunctionArray[3] = PopulateRGBFromGBRG;
            break;
        case RGGB:
            deyaringFunctionArray[0] = PopulateRGBFromRGGB;
            deyaringFunctionArray[1] = PopulateRGBFromGRBG;
            deyaringFunctionArray[2] = PopulateRGBFromGBRG;
            deyaringFunctionArray[3] = PopulateRGBFromBGGR;
            break;
        default:
            return 1;
    }

    return 0;
}

/**
 * @brief Change BGR frame to RGB frame
 * @param[in]   frame           BGR frame
 * @param[in]   frameWidth      BGR frame width
 * @param[in]   frameHeight     BGR frame height
 */
static void bgr2rgb(uint8_t* frame,
                    uint32_t frameWidth,
                    uint32_t frameHeight)
{
    uint32_t frame_size = frameWidth * frameHeight * 3U;
    for (uint32_t i = 0U; i < frame_size; i+=3U) {
        uint8_t tmp = frame[i];
        frame[i] = frame[i +2];
        frame[i + 2] = tmp;
    }
}

int CropAndDebayer(
    const uint8_t* rawImgData,
    uint32_t rawImgWidth,
    uint32_t rawImgHeight,
    uint32_t rawImgCropOffsetX,
    uint32_t rawImgCropOffsetY,
    uint8_t* rgbImgData,
    uint32_t rgbImgWidth,
    uint32_t rgbImgHeight,
    enum ColourFilter bayerFormat)
{
    const uint32_t rawImgStep = rawImgWidth;
    const uint32_t rgbImgStep = rgbImgWidth * 3;

    /* Infer the tile pattern at which we will begin based on offsets. */
    enum ColourFilter startingPattern = GetStartingTilePattern(bayerFormat,
                                                          rawImgCropOffsetX,
                                                          rawImgCropOffsetY);

    if (Invalid == startingPattern) {
        // printf("Invalid bayer pattern\n");
        return 1;
    }

    /* Get the order in which the row populating functions will be called. */
    DebayerRowPopulateFunction functionArray[4];
    if (GetDebayeringFunctionOrder(startingPattern, functionArray)) {
        // printf("Failed to get debayering function sequence\n");
        return 1;
    }

    /* Traverse through the raw and populate the RGB image. */
    for (uint32_t j = 0; j < rgbImgHeight; j += 2) {

        const uint8_t *pSrc = rawImgData + rawImgCropOffsetX +
                             (rawImgStep * (rawImgCropOffsetY + j));
        uint8_t* pDst = rgbImgData + (rgbImgStep * j);


        for (uint32_t i = 0; i < rgbImgWidth; i += 2) {

            functionArray[0](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;

            functionArray[1](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;
        }

        pSrc = rawImgData + rawImgCropOffsetX +
               (rawImgStep * (rawImgCropOffsetY + j + 1));
        pDst = rgbImgData + (rgbImgStep * (j + 1));

        for (uint32_t i = 0; i < rgbImgWidth; i += 2) {

            functionArray[2](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;

            functionArray[3](pSrc, pDst, rawImgStep);
            ++pSrc;
            pDst += 3;
        }
    }

    bgr2rgb(rgbImgData, rgbImgWidth, rgbImgHeight);

    return 0;
}
