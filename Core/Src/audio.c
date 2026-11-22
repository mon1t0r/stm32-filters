#include "audio.h"

#include <string.h>

#include "main.h"
#include "cs42l51.h"
#include "mem_attr.h"

extern SAI_HandleTypeDef hsai_BlockA1; /* TX, master */
extern SAI_HandleTypeDef hsai_BlockB1; /* RX, slave */

/* int16_t sample count in one half */
#define BLOCK_SAMPLES (AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS)
#define BUF_SAMPLES   (2U * BLOCK_SAMPLES)

/* Ring buffers from two halves: DMA fills one, MCU processes the other.
 * Align to 32 bytes for D-Cache in the future */
static int16_t g_tx_buf[BUF_SAMPLES] __ATTR_RAM_D1 __ALIGNED(32);
static int16_t g_rx_buf[BUF_SAMPLES] __ATTR_RAM_D1 __ALIGNED(32);

/* Audio error count */
volatile uint32_t g_audio_err_cnt = 0;

static void audio_process(const int16_t *in, int16_t *out, uint32_t frames)
{
    /* TODO: Filter process */
    memcpy(out, in, frames * AUDIO_CHANNELS * sizeof(*out));
}

static void process_half(int half)
{
    audio_process(
        &g_rx_buf[half * BLOCK_SAMPLES],
        &g_tx_buf[half * BLOCK_SAMPLES],
        AUDIO_FRAMES_PER_BLOCK
    );
}

/* DMA callbacks */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B) {
        process_half(0);
    }
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B) {
        process_half(1);
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

    /* Set up DMA */
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
