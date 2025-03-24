/*
 * SPDX-FileCopyrightText: Copyright 2022 Arm Limited and/or its
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

#include <stddef.h>
#include "AudioSource.hpp"

#define NUMBER_OF_FILES (1U)

extern const int16_t audio0[16000];

static const char* audio_clip_filenames[] = {
    "ks_down.wav",
};
 
static const int16_t* audio_clip_arrays[] = {
    audio0,
};
 
static const size_t audio_clip_sizes[NUMBER_OF_FILES] = {
    16000,
};

bool open_audio_source(const uint32_t idx)
{
    if(idx < NUMBER_OF_FILES) {
        return true;
    }
    return false;
}

void close_audio_source(const uint32_t idx)
{
    (void)idx;
}

const char* get_audio_name(const uint32_t idx)
{
    if (idx < NUMBER_OF_FILES) {
        return audio_clip_filenames[idx];
    }
    return nullptr;
}
 
const int16_t* get_audio_array(const uint32_t idx)
{
    if (idx < NUMBER_OF_FILES) {
        return audio_clip_arrays[idx];
    }
    return nullptr;
}
 
uint32_t get_audio_array_size(const uint32_t idx)
{
    if (idx < NUMBER_OF_FILES) {
        return audio_clip_sizes[idx];
    }
    return 0;
}
 