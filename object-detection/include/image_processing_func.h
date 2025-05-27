/*---------------------------------------------------------------------------
 * Copyright (c) 2025 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#ifndef IMAGE_PROCESSING_FUNC_H__
#define IMAGE_PROCESSING_FUNC_H__

#include <stdint.h>

/* Bayer pattern definitions */
#define BAYER_PATTERN_RGGB    0 
#define BAYER_PATTERN_BGGR    1
#define BAYER_PATTERN_GRBG    2
#define BAYER_PATTERN_GBRG    3

#ifdef __cplusplus
extern "C" {
#endif

typedef int bayer_pattern_t;

/**
 * @brief Perform debayering on a raw Bayer image.
 *
 * Converts a single-channel Bayer-pattern image into a full RGB image.
 *
 * The raw Bayer image must follow one of the standard 2x2 Bayer patterns and
 * use 8-bit grayscale pixels. The output RGB image is stored as 24-bit RGB
 * (3 bytes per pixel, in R-G-B order). Supports optional red/blue channel swap.
 *
 * Note: The outer 1-pixel border is skipped to avoid accessing out-of-bounds
 * pixels. These edges will remain unprocessed unless handled separately.
 *
 * @param[in]  raw      Pointer to the raw Bayer image buffer (size: width × height).
 * @param[out] rgb      Pointer to the output RGB buffer (size: width × height × 3).
 * @param[in]  width    Width of the image in pixels.
 * @param[in]  height   Height of the image in pixels.
 * @param[in]  pattern  Bayer pattern used in the raw image.
 * @param[in]  swap_rb  If non-zero, swap the red and blue channels in the output.
 */
void image_debayer(const uint8_t *raw,
                   uint8_t *rgb,
                   int width,
                   int height,
                   bayer_pattern_t pattern,
                   int swap_rb);

/**
 * @brief Resize an RGB888 image.
 *
 * This function resizes a RGB888 image (3 bytes per pixel) to a new resolution.
 *
 * @param[in]  src         Pointer to the input RGB888 buffer.
 * @param[in]  src_width   Width of the source image.
 * @param[in]  src_height  Height of the source image.
 * @param[out] dst         Pointer to the output RGB888 buffer.
 * @param[in]  dst_width   Width of the resized output image.
 * @param[in]  dst_height  Height of the resized output image.
 */
void image_resize(const uint8_t *src,
                  int src_width,
                  int src_height,
                  uint8_t *dst,
                  int dst_width,
                  int dst_height);

/**
 * @brief Copy a smaller or equally sized image into a destination frame buffer at a given offset.
 *
 * Assumes that the source image fits completely within the destination at the specified offset.
 * Assumes RGB888 format for the source image and for the destination framebuffer.
 *
 * @param[in]  src          Pointer to the source image buffer.
 * @param[in]  src_width    Width of the source image in pixels.
 * @param[in]  src_height   Height of the source image in pixels.
 * @param[out] dst          Pointer to the destination framebuffer.
 * @param[in]  dst_width    Width of the destination framebuffer in pixels.
 * @param[in]  dst_height   Height of the destination framebuffer in pixels.
 * @param[in]  x_offset     X offset in the destination framebuffer to start copying.
 * @param[in]  y_offset     Y offset in the destination framebuffer to start copying.
 */
void image_copy_to_framebuffer(const uint8_t *src,
                               int src_width,
                               int src_height,
                               uint8_t *dst,
                               int dst_width,
                               int dst_height,
                               int x_offset,
                               int y_offset);

#ifdef __cplusplus
}
#endif
#endif /* IMAGE_PROCESSING_FUNC_H__ */
