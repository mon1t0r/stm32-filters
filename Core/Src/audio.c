#include "audio.h"

#include <string.h>

#include "stm32h7xx_hal_def.h"
#include "mem_attr.h"
#include "cs42l51.h"

#include "filter.h"

extern SAI_HandleTypeDef hsai_BlockA1; /* TX, master */
extern SAI_HandleTypeDef hsai_BlockB1; /* RX, slave */

#define BLOCK_SAMPLES (AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS)
#define BUF_SAMPLES   (BLOCK_SAMPLES * sizeof(uint16_t))

/* Ring buffers from two halves: DMA fills one, MCU processes the other */
static int16_t g_tx_buf[BUF_SAMPLES] __ATTR_RAM_D1;
static int16_t g_rx_buf[BUF_SAMPLES] __ATTR_RAM_D1;

/* Audio error count */
volatile uint32_t g_audio_err_cnt = 0;

static inline void audio_process_half(int half)
{
    filter_process(
        &g_rx_buf[half * BLOCK_SAMPLES],
        &g_tx_buf[half * BLOCK_SAMPLES]
    );
}

/* DMA callbacks */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B) {
        audio_process_half(0);
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B) {
        audio_process_half(1);
    }
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    (void) hsai;
    g_audio_err_cnt++;
}

int audio_init(void)
{
    int r;

    /* Zero out buffers */
    memset(g_tx_buf, 0, sizeof(g_tx_buf));
    memset(g_rx_buf, 0, sizeof(g_rx_buf));

    /* Set up SAI DMA */
    r = HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t *) g_rx_buf, BUF_SAMPLES);
    if(r != HAL_OK) {
        return AUDIO_ERROR;
    }

    r = HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *) g_tx_buf, BUF_SAMPLES);
    if(r != HAL_OK) {
        return AUDIO_ERROR;
    }

    HAL_Delay(10);

    /* Initialize and start codec */
    r = cs42l51_init();
    if(r != CS42L51_OK) {
        return AUDIO_ERROR;
    }

    r = cs42l51_start();
    if(r != CS42L51_OK) {
        return AUDIO_ERROR;
    }

    return AUDIO_OK;
}

void audio_stop(void)
{
    cs42l51_stop();
    HAL_SAI_DMAStop(&hsai_BlockA1);
    HAL_SAI_DMAStop(&hsai_BlockB1);
}
