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
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include <stdint.h>
#include "cmsis_compiler.h"
#include "image_processing_func.h"

/* Debayering on a raw Bayer image. */
__WEAK void image_debayer(const uint8_t *raw,
                          uint8_t *rgb,
                          int width,
                          int height,
                          bayer_pattern_t pattern,
                          int swap_rb) {
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      int idx = y * width + x;
      int r = 0, g = 0, b = 0;

      int is_even_row = (y & 1) == 0;
      int is_even_col = (x & 1) == 0;

      switch (pattern) {
        case BAYER_PATTERN_RGGB:
          if (is_even_row) {
            if (is_even_col) {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          } else {
            if (is_even_col) {
              // Green (on blue row)
              g = raw[idx];
              r = (raw[idx - width] + raw[idx + width]) >> 1;
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
            } else {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          }
          break;

        case BAYER_PATTERN_BGGR:
          if (is_even_row) {
            if (is_even_col) {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          } else {
            if (is_even_col) {
              // Green (on red row)
              g = raw[idx];
              b = (raw[idx - width] + raw[idx + width]) >> 1;
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
            } else {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          }
          break;

        case BAYER_PATTERN_GRBG:
          if (is_even_row) {
            if (is_even_col) {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            } else {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          } else {
            if (is_even_col) {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          }
          break;

        case BAYER_PATTERN_GBRG:
          if (is_even_row) {
            if (is_even_col) {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            } else {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          } else {
            if (is_even_col) {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          }
          break;
      }

      int out_idx = (y * width + x) * 3;
      if (swap_rb == 0) {
        rgb[out_idx + 0] = (uint8_t)r;
        rgb[out_idx + 1] = (uint8_t)g;
        rgb[out_idx + 2] = (uint8_t)b;
      } else {
        rgb[out_idx + 0] = (uint8_t)b;
        rgb[out_idx + 1] = (uint8_t)g;
        rgb[out_idx + 2] = (uint8_t)r;
      }
    }
  }
}

#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MASK  (FP_ONE - 1)

__WEAK void image_resize(const uint8_t *src,
                         int src_width,
                         int src_height,
                         uint8_t *dst,
                         int dst_width,
                         int dst_height) {
  const int bpp = 3;  // bytes per pixel
  int x_ratio = ((src_width - 1) << FP_SHIFT) / (dst_width - 1);
  int y_ratio = ((src_height - 1) << FP_SHIFT) / (dst_height - 1);

  for (int y = 0; y < dst_height; ++y) {
    int src_y_fp = y * y_ratio;
    int y0 = src_y_fp >> FP_SHIFT;
    int y1 = (y0 < src_height - 1) ? y0 + 1 : y0;
    int wy = src_y_fp & FP_MASK;

    for (int x = 0; x < dst_width; ++x) {
      int src_x_fp = x * x_ratio;
      int x0 = src_x_fp >> FP_SHIFT;
      int x1 = (x0 < src_width - 1) ? x0 + 1 : x0;
      int wx = src_x_fp & FP_MASK;

      int rgb[3];

      for (int c = 0; c < 3; ++c) {
        int idx00 = (y0 * src_width + x0) * bpp + c;
        int idx01 = (y0 * src_width + x1) * bpp + c;
        int idx10 = (y1 * src_width + x0) * bpp + c;
        int idx11 = (y1 * src_width + x1) * bpp + c;

        int top = ((FP_ONE - wx) * src[idx00] + wx * src[idx01]) >> FP_SHIFT;
        int bot = ((FP_ONE - wx) * src[idx10] + wx * src[idx11]) >> FP_SHIFT;
        rgb[c] = ((FP_ONE - wy) * top + wy * bot) >> FP_SHIFT;
      }

      int out_idx = (y * dst_width + x) * bpp;

      dst[out_idx + 0] = (uint8_t)rgb[0]; // R
      dst[out_idx + 1] = (uint8_t)rgb[1]; // G
      dst[out_idx + 2] = (uint8_t)rgb[2]; // B
    }
  }
}

__WEAK void image_copy_to_framebuffer(const uint8_t *src,
                                      int src_width,
                                      int src_height,
                                      uint8_t *dst,
                                      int dst_width,
                                      int dst_height,
                                      int x_offset,
                                      int y_offset) {
  int bpp = 3; // bytes per pixel

  // Copy row by row
  for (int y = 0; y < src_height; ++y) {
    int dst_y = y + y_offset;
    int dst_x = x_offset;

    int dst_row_offset = (dst_y * dst_width + dst_x) * bpp;
    int src_row_offset = (y * src_width) * bpp;

    for (int x = 0; x < src_width * bpp; ++x) {
      dst[dst_row_offset + x] = src[src_row_offset + x];
    }
  }
}
