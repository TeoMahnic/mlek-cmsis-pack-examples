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

#ifndef VIDEO_CONFIGURATION_HPP
#define VIDEO_CONFIGURATION_HPP

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>Camera Configuration
// =======================

//  <o>Camera Frame Width
//  <i> Define the camera frame width.
//  <i> Default: 0
#ifndef CAMERA_FRAME_WIDTH
#define CAMERA_FRAME_WIDTH          1280
#endif

//  <o>Camera Frame Height
//  <i> Define the camera frame height.
//  <i> Default: 0
#ifndef CAMERA_FRAME_HEIGHT
#define CAMERA_FRAME_HEIGHT         720
#endif

//  <s>Frame Buffer Section Name
//  <i> Define the name of the camera frame buffer section
//  <i> Default: ".bss.camera_frame_buf"
#ifndef CAMERA_FRAME_BUF_SECTION
#define CAMERA_FRAME_BUF_SECTION    ".bss.camera_frame_buf"
#endif

//  <o>Frame Buffer Alignment
//  <i> Define the camera frame buffer alignment in bytes
//  <i> Default: 32
#ifndef CAMERA_FRAME_BUF_ALIGNMENT
#define CAMERA_FRAME_BUF_ALIGNMENT  32
#endif

//  <o>Frame Type <0=>RAW <1=>RGB
//  <i> Define whether camera frame is raw or RGB.
//  <i> RGB888 is assumed for non-raw frames.
//  <i> Default: 0
#ifndef CAMERA_FRAME_TYPE
#define CAMERA_FRAME_TYPE           0
#endif

//  <o>Frame Bayer Pattern <0=>RGGB <1=>BGGR <2=>GRBG <3=>GBRG
//  <i> Define the raw camera frame Bayer pattern.
//  <i> Default: 3
#ifndef CAMERA_FRAME_BAYER
#define CAMERA_FRAME_BAYER          3
#endif

// </h>

// <h>Display Configuration
// ========================

//  <o>Display Frame Width
//  <i> Defines the display frame width.
//  <i> Default: 0
#ifndef DISPLAY_FRAME_WIDTH
#define DISPLAY_FRAME_WIDTH         480
#endif

//  <o>Display Frame Height
//  <i> Defines the display frame height.
//  <i> Default: 0
#ifndef DISPLAY_FRAME_HEIGHT
#define DISPLAY_FRAME_HEIGHT        800
#endif

//  <s>Frame Buffer Section Name
//  <i> Define the name of the display frame buffer section
//  <i> Default: ".bss.lcd_frame_buf"
#ifndef DISPLAY_FRAME_BUF_SECTION
#define DISPLAY_FRAME_BUF_SECTION   ".bss.lcd_frame_buf"
#endif

//  <o>Frame Buffer Alignment
//  <i> Define the display frame buffer alignment in bytes
//  <i> Default: 32
#ifndef DISPLAY_FRAME_BUF_ALIGNMENT
#define DISPLAY_FRAME_BUF_ALIGNMENT 32
#endif

// </h>

#endif /* VIDEO_CONFIGURATION_HPP */
