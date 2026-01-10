#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include "main.h"
#include "fatfs.h"
#include <stdbool.h>

// 오디오 포맷 정의
#define AUDIO_SAMPLE_RATE    16000
#define AUDIO_CHANNELS       1   // Mono
#define AUDIO_BITS_PER_SAMPLE 16

// 상태 머신 정의
typedef enum {
    AUDIO_STATE_IDLE,
    AUDIO_STATE_RECORDING,
    AUDIO_STATE_STOPPING,
    AUDIO_STATE_ERROR
} AudioState_t;

// 공개 함수
void Audio_Init(void);
void Audio_StartRecording(const char* filename);
void Audio_StopRecording(void);
void Audio_Process(void); // 오디오 태스크에서 주기적으로 호출
AudioState_t Audio_GetState(void);

// I2S DMA 콜백 (main.c 또는 stm32f4xx_it.c에서 호출 필요)
void Audio_I2S_HalfCpltCallback(void);
void Audio_I2S_CpltCallback(void);

#endif
