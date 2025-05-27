/*
 * SPDX-FileCopyrightText: Copyright 2022, 2024 Arm Limited and/or its
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

#ifndef BUF_ATTRIBUTES_HPP
#define BUF_ATTRIBUTES_HPP

#include "AppConfiguration.hpp"   /* Application configuration  */
#include "VideoConfiguration.hpp" /* Video configuration        */

// RGB Image Buffer Section Name
// Define the name of the RGB image buffer section
// Default: ".bss.rgb_image_buf"
#ifndef RGB_IMAGE_BUF_SECTION
#define RGB_IMAGE_BUF_SECTION       ".bss.rgb_image_buf"
#endif
// RGB Image Buffer Alignment
// Define the RGB image buffer alignment in bytes
// Default: 4
#ifndef RGB_IMAGE_BUF_ALIGNMENT
#define RGB_IMAGE_BUF_ALIGNMENT     4
#endif


// ML Image Width
// Define the width of the image to be used for ML inference.
// Default: 192
#ifndef ML_IMAGE_WIDTH
#define ML_IMAGE_WIDTH              192
#endif
// ML Image Height
// Define the height of the image to be used for ML inference.
// Default: 192
#ifndef ML_IMAGE_HEIGHT
#define ML_IMAGE_HEIGHT             192
#endif
// ML Image Buffer Section Name
// Define the name of the section for the ML image buffer
// Default: ".bss.ml_image_buf"
#ifndef ML_IMAGE_BUF_SECTION
#define ML_IMAGE_BUF_SECTION        ".bss.ml_image_buf"
#endif
// ML Image Buffer Alignment
// Define the alignment in bytes for the ML image buffer
// Default: 4
#ifndef ML_IMAGE_BUF_ALIGNMENT
#define ML_IMAGE_BUF_ALIGNMENT      4
#endif

/* Attributes applied to the activation buffer object */
#define ACTIVATION_BUF_ATTRIBUTE \
  __attribute__((section(ACTIVATION_BUF_SECTION), aligned(ACTIVATION_BUF_ALIGNMENT)))

/* Attributes applied to the ML image buffer object */
#define NN_MODEL_BUF_ATTRIBUTE \
  __attribute__((section(NN_MODEL_BUF_SECTION), aligned(NN_MODEL_BUF_ALIGNMENT)))

#define MODEL_TFLITE_ATTRIBUTE  NN_MODEL_BUF_ATTRIBUTE

/* Attributes applied to the camera frame buffer object */
#define CAMERA_FRAME_BUF_ATTRIBUTE \
  __attribute__((section(CAMERA_FRAME_BUF_SECTION), aligned(CAMERA_FRAME_BUF_ALIGNMENT)))

/* Attributes applied to the display frame buffer object */
#define DISPLAY_FRAME_BUF_ATTRIBUTE \
  __attribute__((section(DISPLAY_FRAME_BUF_SECTION), aligned(DISPLAY_FRAME_BUF_ALIGNMENT)))

/* Attributes applied to the RGB image buffer object */
#define RGB_IMAGE_BUF_ATTRIBUTE \
  __attribute__((section(RGB_IMAGE_BUF_SECTION), aligned(RGB_IMAGE_BUF_ALIGNMENT)))

/* Attributes applied to the ML image buffer object */
#define ML_IMAGE_BUF_ATTRIBUTE \
  __attribute__((section(ML_IMAGE_BUF_SECTION), aligned(ML_IMAGE_BUF_ALIGNMENT)))

#endif /* BUF_ATTRIBUTES_HPP */
