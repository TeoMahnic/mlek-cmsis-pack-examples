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
#include "cmsis_vstream.h"
#include "cmsis_os2.h"

#include "log_macros.h"
#include "arm_math.h"

/* Define audio input and inference buffers */
#define AUDIO_IN_BLOCK_COUNT    (2)
#ifndef CMSIS_VSTREAM_AUDIO_IN_MONO     // If audio is captured in stereo
#define AUDIO_IN_BLOCK_SAMPLES  (16000) // 0.5 seconds at 16 kHz (in stereo)
#else                                   // If audio is captured in mono
#define AUDIO_IN_BLOCK_SAMPLES  (8000)  // 0.5 seconds at 16 kHz (in mono)
#endif
#define AUDIO_IN_BLOCK_SIZE     (AUDIO_IN_BLOCK_SAMPLES * sizeof(int16_t))

#define INFER_BLOCK_COUNT       (2)
#define INFER_BLOCK_SAMPLES     (8000)  // 0.5 seconds at 16 kHz (conditioned and mono)
#define INFER_BLOCK_SIZE        (INFER_BLOCK_SAMPLES * sizeof(int16_t))

int16_t audioInBuffer[AUDIO_IN_BLOCK_SAMPLES * AUDIO_IN_BLOCK_COUNT];
int16_t inferBuffer[INFER_BLOCK_SAMPLES * INFER_BLOCK_COUNT];

/* Reference to the underlying CMSIS vStream driver */
extern vStreamDriver_t          Driver_vStreamAudioIn;
#define vStream_AudioIn       (&Driver_vStreamAudioIn)

/* Audio processing functions */
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
  Process buffer audio input data and condition it for buffer for inference
*/
void audio_capture (void *arg) {
  int16_t *buf;
  int32_t audioGain   = 0;
  int32_t audioOffset = 0;

  /* Initialize audio in stream and set the receive buffer */
  vStream_AudioIn->Initialize(AudioDrv_Event_Callback);
  vStream_AudioIn->SetBuf(audioInBuffer, AUDIO_IN_BLOCK_COUNT * AUDIO_IN_BLOCK_SIZE, AUDIO_IN_BLOCK_SIZE);

  /* Start audio receiver */
  vStream_AudioIn->Start(VSTREAM_MODE_CONTINUOUS);

  while(1) {
      /* Wait for flag from audio callback */
      osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);

      /* Process block of currently received audio samples */
      buf = (int16_t *)vStream_AudioIn->GetBlock();

      /* Recalculate offset and gain */
      audioOffset = CalculateOffset(buf, AUDIO_IN_BLOCK_SAMPLES);
      audioGain = CalculateScale(buf, AUDIO_IN_BLOCK_SAMPLES);

      /* Apply offset and scaling factor (gain) to each audio sample */
      ApplyGainAndOffset(buf, AUDIO_IN_BLOCK_SAMPLES, audioOffset, audioGain);

      /* Move 2nd block of inference buffer data to the beginning */
      memcpy(inferBuffer, &inferBuffer[INFER_BLOCK_SAMPLES], INFER_BLOCK_SAMPLES * (INFER_BLOCK_COUNT - 1) * 2);

      /* Populate the last block of the buffer for inference with freshly captured and conditioned audio data */
#ifndef CMSIS_VSTREAM_AUDIO_IN_MONO
      ConvertToMono(&inferBuffer[INFER_BLOCK_SAMPLES * (INFER_BLOCK_COUNT - 1)], buf, INFER_BLOCK_SAMPLES);
#else
      memcpy(&inferBuffer[INFER_BLOCK_SAMPLES * (INFER_BLOCK_COUNT - 1)], buf, INFER_BLOCK_SIZE);
#endif

      /* Release buffer block to vStream driver */
      vStream_AudioIn->ReleaseBlock();

      /* Mono buffer is ready, start processing it */
      osThreadFlagsSet(tid_app_main, 0x0001);
  }
}

/*
  Convert stereo audio data to mono.
  The function takes stereo audio data (2 channels) and converts it to mono by
  averaging the two channels.
*/
static void ConvertToMono(int16_t *monoData, int16_t *stereoData, uint32_t n_samples)
{
    int16_t* pIn  = stereoData;
    int16_t* pOut = monoData;

    for (int i = 0; i < n_samples; i++){
        pOut[i] = ((pIn[0] >> 1) + (pIn[1] >> 1));

        pIn += 2;
    }
}

/*
  Calculate offset correction value.
  Offset determines how much the audio signal should be shifted up or down to
  center it around zero.
*/
static int32_t CalculateOffset(int16_t *audioData, uint32_t sampleCount)
{
    int16_t audioMean = 0;
    arm_mean_q15(audioData, sampleCount, &audioMean);
    return static_cast<int32_t>(0 - audioMean);
}

/*
  Calculate a scaling factor for audio normalization.
  The scaling factor is used to normalize or amplify the audio signal to a
  desired range (avoiding over-amplifying noise or silence).
*/
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

/*
  Apply gain and offset to the audio data.
  Applies linear transformation with gain and offset and ensures that the values
  stay within the range of int16_t.
*/
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

bool open_audio_source(const uint32_t idx)
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

void close_audio_source(const uint32_t idx)
{
    /* Unused, audio source is always the same */
    (void)idx;
}

const char* get_audio_name(const uint32_t idx)
{
    /* This is audio from hardware audio stream */
    return "Live Audio Stream";
}

const int16_t* get_audio_array(const uint32_t idx)
{
    /* Inference buffer is the audio source array for inference */
    return inferBuffer;
}

uint32_t get_audio_array_size(const uint32_t idx)
{
    /* Return number of elements in audio array */
    return INFER_BLOCK_SAMPLES * INFER_BLOCK_COUNT;
}
