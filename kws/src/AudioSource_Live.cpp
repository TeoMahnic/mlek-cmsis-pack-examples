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

#include <algorithm>
#include <cstdint>

#include "AudioSource.hpp" 
#include "audio_drv.h"
#include "cmsis_os2.h"

#include "arm_math.h"
#include "log_macros.h"

#define STEREO_BLOCK_COUNT   (2)
#define STEREO_BLOCK_SAMPLES (16000)
#define STEREO_BLOCK_SIZE    (STEREO_BLOCK_SAMPLES * 2)
#define MONO_BLOCK_COUNT     (2)
#define MONO_BLOCK_SAMPLES   (8000)
#define MONO_BLOCK_SIZE      (MONO_BLOCK_SAMPLES * 2)

int16_t stereoBuffer[STEREO_BLOCK_SAMPLES * STEREO_BLOCK_COUNT];
int16_t monoBuffer[MONO_BLOCK_SAMPLES * MONO_BLOCK_COUNT];

uint32_t stereo_block;
uint32_t mono_block;

static int32_t CalculateOffset(int16_t *audioData, uint32_t sampleCount);
static int32_t CalculateScale(int16_t *audioData, uint32_t sampleCount);
static void ApplyGainAndOffset(int16_t *audioData, uint32_t sampleCount, int32_t audioOffset, int32_t audioScale);
static void ConvertToMono(int16_t *stereoData, int16_t *monoData, uint32_t sampleCount);

osThreadId_t tid_app_main = NULL;
osThreadId_t tid_audio_capture = NULL;

void AudioDrv_Event_Callback (uint32_t event) {
  (void)event;

  osThreadFlagsSet(tid_audio_capture, 0x0001);
}

/**
  Process stereo buffer audio data and convert it to fit into mono buffer
*/
void audio_capture (void *arg) {
  int32_t audioGain   = 0;
  int32_t audioOffset = 0;

  stereo_block = 0;
  mono_block = 0;

  AudioDrv_Initialize(AudioDrv_Event_Callback);
  AudioDrv_Configure(AUDIO_DRV_INTERFACE_RX, 1, 16, 16000);
  AudioDrv_SetBuf(AUDIO_DRV_INTERFACE_RX, stereoBuffer, STEREO_BLOCK_COUNT, STEREO_BLOCK_SIZE);

  /* Start audio receiver */
  AudioDrv_Control(AUDIO_DRV_CONTROL_RX_ENABLE);

  while(1) {
      /* Wait for flag from audio callback */
      osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);

      /* Process block of currently received audio samples */

      /* Recalculate offset and gain */
      audioOffset = CalculateOffset(&stereoBuffer[stereo_block * STEREO_BLOCK_SAMPLES], STEREO_BLOCK_SAMPLES);
      audioGain = CalculateScale(&stereoBuffer[stereo_block * STEREO_BLOCK_SAMPLES], STEREO_BLOCK_SAMPLES);

      /* Apply offset and scaling factor (gain) to each audio sample */
      ApplyGainAndOffset(&stereoBuffer[stereo_block * STEREO_BLOCK_SAMPLES], STEREO_BLOCK_SAMPLES, audioOffset, audioGain);

      /* Move mono buffer data to the beginning (shift by one block) */
      memcpy(monoBuffer, &monoBuffer[MONO_BLOCK_SAMPLES], MONO_BLOCK_SAMPLES * (MONO_BLOCK_COUNT - 1) * 2);

      /* Populate the last block of the mono buffer from the freshly captured stereo audio */
      ConvertToMono(&monoBuffer[MONO_BLOCK_SAMPLES * (MONO_BLOCK_COUNT - 1)], &stereoBuffer[STEREO_BLOCK_SAMPLES * stereo_block], MONO_BLOCK_SAMPLES);

      /* Set next block to process */
      stereo_block = (stereo_block + 1) % STEREO_BLOCK_COUNT;

      /* Mono buffer is ready, start processing it */
      osThreadFlagsSet(tid_app_main, 0x0001);
  }
}


static void ConvertToMono(int16_t *monoData, int16_t *stereoData, uint32_t n_samples)
{
    int16_t* pIn  = stereoData;
    int16_t* pOut = monoData;

    for (int i = 0; i < n_samples; i++){
        pOut[i] = ((pIn[0] >> 1) + (pIn[1] >> 1));

        pIn += 2;
    }
}

static int32_t CalculateOffset(int16_t *audioData, uint32_t sampleCount)
{
    int16_t audioMean = 0;
    arm_mean_q15(audioData, sampleCount, &audioMean);
    return static_cast<int32_t>(0 - audioMean);
}

static int32_t CalculateScale(int16_t *audioData, uint32_t sampleCount)
{
    /* Define the desired signal span to scale our input signal to. It can be based on
     * the training data set, or close to std::numeric_limits<int16_t>::max()/2; */
    constexpr int32_t desirableSignalSpan = 18000;

    /* Maximum scaling factor. A factor bigger than this may amplify noise which can
     * lead to false detections. */
    constexpr int32_t maxScale = 25;

    int16_t audioMin = 0;
    arm_min_no_idx_q15(audioData, sampleCount, &audioMin);

    int16_t audioMax = 0;
    arm_max_no_idx_q15(audioData, sampleCount, &audioMax);

    int32_t audioScale = desirableSignalSpan / (audioMax - audioMin);

    /* We don't want random silence to be amplified too much; we limit
     * the gain */
    if (audioScale > maxScale) {
        audioScale = maxScale;
    } else if (audioScale < 1) {
        audioScale = 1;
    }

    return audioScale;
}

static void ApplyGainAndOffset(int16_t *audioData, uint32_t sampleCount, int32_t audioOffset, int32_t audioScale)
{
    debug("Scale: %d; Offset: %d\n", audioScale, audioOffset);
    int16_t* buf = static_cast<int16_t*>(audioData);

    /* Apply offset first and then gain */
    for (uint32_t i = 0; i < sampleCount; ++i) {
        auto& sample         = buf[i];
        int32_t modified_val = (static_cast<int32_t>(sample) + audioOffset) * audioScale;

        /* Clip the high end */
        modified_val = std::min<int32_t>(modified_val,
                                         static_cast<int32_t>(std::numeric_limits<int16_t>::max()));

        /* Clip the low end */
        modified_val = std::max<int32_t>(modified_val,
                                         static_cast<int32_t>(std::numeric_limits<int16_t>::min()));

        sample = static_cast<int16_t>(modified_val);
    }
}

bool is_file_available(const uint32_t idx)
{
    uint32_t flags;

    if (tid_audio_capture == NULL) {
      /* Get application thread ID */
      tid_app_main = osThreadGetId();

      /* Create audio capture thread */
      tid_audio_capture = osThreadNew(audio_capture, NULL, NULL);
    }

    /* Wait for flag from audio capture thread (2 sec timeout) */
    flags = osThreadFlagsWait(0x0001, osFlagsWaitAny, 2000);

    if (flags == osFlagsErrorTimeout) {
        /* Capture thread failed to retrieve audio samples */
        return false;
    }

    return true;
}

const char* get_filename(const uint32_t idx)
{
    return "Live Audio Stream";
}

const int16_t* get_audio_array(const uint32_t idx)
{
    return monoBuffer;
}

uint32_t get_audio_array_size(const uint32_t idx)
{
    /* This is actually number of elements in audio array */
    return MONO_BLOCK_SAMPLES * MONO_BLOCK_COUNT;
}
