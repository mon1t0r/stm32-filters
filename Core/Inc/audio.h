#ifndef __AUDIO_H
#define __AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define AUDIO_OK    0
#define AUDIO_ERROR (-1)

/* Same as in main.c SAI init */
#define AUDIO_SAMPLE_RATE SAI_AUDIO_FREQUENCY_8K
#define AUDIO_CHANNELS    2

/* 1ms delay */
#define AUDIO_FRAMES_PER_BLOCK (AUDIO_SAMPLE_RATE / 1000)

/* SAI error count (overrun/underrun) */
/* Used while debugging */
extern volatile uint32_t g_audio_err_cnt;

int audio_init(void);

void audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_H */
