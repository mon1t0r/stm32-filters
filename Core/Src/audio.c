#include "audio.h"

#include <string.h>

#include "main.h"
#include "cs42l51.h"
#include "uart.h"
#include "mem_attr.h"

extern SAI_HandleTypeDef hsai_BlockA1; /* TX, master */
extern SAI_HandleTypeDef hsai_BlockB1; /* RX, slave */

/* int16_t sample count in one half */
#define BLOCK_SAMPLES (AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS)
#define BUF_SAMPLES   (2 * BLOCK_SAMPLES)

/* High pass filter weights - exported from MatLab */
static const double g_weights[] = {
    0, 0.000100054867683376, 0.000195306467590836, 0.000278228897919648,
    0.000341239916047361, 0.000377062419547929, 0.000379290873633016,
    0.00034315340414915, 0.000266404373662877, 0.000150228868749273, 0,
    -0.000174288535190983, -0.000358082033506903, -0.00053294260507404,
    -0.00067790097481392, -0.000771445564532916, -0.000794008800071546,
    -0.000730710503327413, -0.000574036815592408, -0.000326084881777902, 0,
    0.000379720347244069, 0.000778282027924939, 0.00115309208478377,
    0.00145760690025604, 0.00164625243067504, 0.00167996322939341,
    0.00153174726862912, 0.0011916011621899, 0.000670092297815684, 0,
    -0.00076443295883395, -0.00155085489187096, -0.00227469729863102,
    -0.00284722751570257, -0.00318508723436458, -0.00322039190822348,
    -0.00291029740755666, -0.00224488099277454, -0.00125225121978014, 0,
    0.00140757079089476, 0.0028366012462644, 0.00413488531561207,
    0.00514634300316576, 0.00572751551848563, 0.00576450371627889,
    0.00518856885354319, 0.00398858794514912, 0.00221872083461348, 0,
    -0.00248492598852695, -0.00500402102216797, -0.00729447925797185,
    -0.00908644529571372, -0.010129923400251, -0.0102224835692276,
    -0.00923509984216992, -0.00713343142886053, -0.00399209811278483, 0,
    0.00454454647723313, 0.00924919813821382, 0.0136528696172327,
    0.0172593143044514, 0.0195765337999176, 0.0201588662670059,
    0.0186481838971248, 0.0148105225646757, 0.00856469562979124, 0,
    -0.0106190345749648, -0.0228618417688564, -0.0361547638142422,
    -0.0498163445711596, -0.0631024574454617, -0.0752578396140503,
    -0.0855699733542568, -0.0934209722912464, -0.0983332263088948,
    0.900045269341973, -0.0983332263088948, -0.0934209722912464,
    -0.0855699733542568, -0.0752578396140503, -0.0631024574454617,
    -0.0498163445711596, -0.0361547638142422, -0.0228618417688564,
    -0.0106190345749648, 0, 0.00856469562979124, 0.0148105225646757,
    0.0186481838971248, 0.0201588662670059, 0.0195765337999176,
    0.0172593143044514, 0.0136528696172327, 0.00924919813821382,
    0.00454454647723313, 0, -0.00399209811278483, -0.00713343142886053,
    -0.00923509984216992, -0.0102224835692276, -0.010129923400251,
    -0.00908644529571372, -0.00729447925797185, -0.00500402102216797,
    -0.00248492598852695, 0, 0.00221872083461348, 0.00398858794514912,
    0.00518856885354319, 0.00576450371627889, 0.00572751551848563,
    0.00514634300316576, 0.00413488531561207, 0.0028366012462644,
    0.00140757079089476, 0, -0.00125225121978014, -0.00224488099277454,
    -0.00291029740755666, -0.00322039190822348, -0.00318508723436458,
    -0.00284722751570257, -0.00227469729863102, -0.00155085489187096,
    -0.00076443295883395, 0, 0.000670092297815684, 0.0011916011621899,
    0.00153174726862912, 0.00167996322939341, 0.00164625243067504,
    0.00145760690025604, 0.00115309208478377, 0.000778282027924939,
    0.000379720347244069, 0, -0.000326084881777902, -0.000574036815592408,
    -0.000730710503327413, -0.000794008800071546, -0.000771445564532916,
    -0.00067790097481392, -0.00053294260507404, -0.000358082033506903,
    -0.000174288535190983, 0, 0.000150228868749273, 0.000266404373662877,
    0.00034315340414915, 0.000379290873633016, 0.000377062419547929,
    0.000341239916047361, 0.000278228897919648, 0.000195306467590836,
    0.000100054867683376, 0, 
};
#define WEIGHTS_LEN (sizeof(g_weights) / sizeof(g_weights[0]))

/* Audio error count */
volatile uint32_t g_audio_err_cnt = 0;

/* Ring buffers from two halves: DMA fills one, MCU processes the other.
 * Align to 32 bytes for D-Cache in the future */
static int16_t g_tx_buf[BUF_SAMPLES] __ATTR_RAM_D1 __ALIGNED(32);
static int16_t g_rx_buf[BUF_SAMPLES] __ATTR_RAM_D1 __ALIGNED(32);

/* Filter history buffer */
static int16_t g_filter_hist[WEIGHTS_LEN * AUDIO_CHANNELS];

static inline int16_t audio_sat(double v)
{
    /* Saturate */
    if(v > 32767.0) {
        return 32767;
    }

    if(v < -32768.0) {
        return -32768;
    }

    return (int16_t) v;
}

static void audio_filter(const int16_t *in, int16_t *out, uint32_t frames)
{
    uint32_t f, ch, w;
    int16_t sample;
    double sum;

    /* Convolution */
    for(f = 0; f < frames; f++) {
        for(ch = 0; ch < AUDIO_CHANNELS; ch++) {
            sum = 0.0;

            for(w = 0; w < WEIGHTS_LEN; w++) {
                if(f < w) {
                    /* Use history samples */
                    sample = g_filter_hist[(WEIGHTS_LEN - 1 + f - w) * AUDIO_CHANNELS + ch];
                } else {
                    /* Use new samples */
                    sample = in[(f - w) * AUDIO_CHANNELS + ch];
                }

                /* MAC */
                sum += sample * g_weights[w];
            }

            /* Write result to audio TX buffer */
            out[f * AUDIO_CHANNELS + ch] = audio_sat(sum);
            uart_write_u16(out[f * AUDIO_CHANNELS + ch]);
        }
    }

    /* Write result to UART */
    //uart_write_mem(out, frames * AUDIO_CHANNELS * sizeof(*out));

    /* Update filter history */
    if(frames >= WEIGHTS_LEN - 1) {
        memcpy(
            g_filter_hist,
            &in[(frames - WEIGHTS_LEN + 1) * AUDIO_CHANNELS],
            (WEIGHTS_LEN - 1) * AUDIO_CHANNELS * sizeof(*in)
        );
    } else {
        memmove(
            g_filter_hist,
            &g_filter_hist[frames * AUDIO_CHANNELS],
            (WEIGHTS_LEN - 1 - frames) * AUDIO_CHANNELS * sizeof(*g_filter_hist)
        );
        memcpy(
            &g_filter_hist[(WEIGHTS_LEN - 1 - frames) * AUDIO_CHANNELS],
            in,
            frames * AUDIO_CHANNELS * sizeof(*in)
        );
    }
}

static inline void audio_process_half(int half)
{
    audio_filter(
        &g_rx_buf[half * BLOCK_SAMPLES],
        &g_tx_buf[half * BLOCK_SAMPLES],
        AUDIO_FRAMES_PER_BLOCK
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
    memset(g_filter_hist, 0, sizeof(g_filter_hist));

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
