#ifndef FILTER_USE_CMSIS

#include "filter.h"

#include <string.h>
#include "filter_weights.h"
#include "uart.h"
#include "audio.h"

#ifdef FILTER_MODE_INT
typedef q63_t fil_sum_t;
#else
typedef float64_t fil_sum_t;
#endif

/* Filter history buffer */
static int16_t g_filter_hist[FILTER_WEIGHTS_LEN * AUDIO_CHANNELS];

/* Current filter index */
static int g_filter_cur = 0;

static inline int16_t audio_sat(fil_sum_t v)
{
    /* Saturate */
    if(v > 32767) {
        return 32767;
    }

    if(v < -32768) {
        return -32768;
    }

    return (int16_t) v;
}

int filter_init(void)
{
    memset(g_filter_hist, 0, sizeof(g_filter_hist));
    return FILTER_OK;
}

void filter_process(const int16_t *in, int16_t *out)
{
    uint32_t f, ch, w;
    int16_t sample;
    fil_sum_t sum;
    int16_t sum_sat;

    /* Convolution */
    for(f = 0; f < AUDIO_FRAMES_PER_BLOCK; f++) {
        for(ch = 0; ch < AUDIO_CHANNELS; ch++) {
            sum = 0;

            for(w = 0; w < FILTER_WEIGHTS_LEN; w++) {
                if(f < w) {
                    /* Use history samples */
                    sample = g_filter_hist[(FILTER_WEIGHTS_LEN - 1 + f - w) * AUDIO_CHANNELS + ch];
                } else {
                    /* Use new samples */
                    sample = in[(f - w) * AUDIO_CHANNELS + ch];
                }

                /* MAC */
                sum += sample * g_filter_weights[g_filter_cur][w];
            }

#ifdef FILTER_MODE_INT
            /* Convert sum from Q2.30 to Q1.15 */
            /* Divide by 32768 */
            sum >>= 15;
#endif

            /* Saturate the sum */
            sum_sat = audio_sat(sum);

            /* Write result to audio TX buffer */
            out[f * AUDIO_CHANNELS + ch] = sum_sat;

            /* Write result to UART */
            uart_write_u16(sum_sat);
        }
    }

    /* Update filter history */
    if(AUDIO_FRAMES_PER_BLOCK >= FILTER_WEIGHTS_LEN - 1) {
        memcpy(
            g_filter_hist,
            &in[(AUDIO_FRAMES_PER_BLOCK - FILTER_WEIGHTS_LEN + 1) * AUDIO_CHANNELS],
            (FILTER_WEIGHTS_LEN - 1) * AUDIO_CHANNELS * sizeof(*in)
        );
    } else {
        memmove(
            g_filter_hist,
            &g_filter_hist[AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS],
            (FILTER_WEIGHTS_LEN - 1 - AUDIO_FRAMES_PER_BLOCK) * AUDIO_CHANNELS * sizeof(*g_filter_hist)
        );
        memcpy(
            &g_filter_hist[(FILTER_WEIGHTS_LEN - 1 - AUDIO_FRAMES_PER_BLOCK) * AUDIO_CHANNELS],
            in,
            AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS * sizeof(*in)
        );
    }
}

void filter_next(void)
{
    g_filter_cur = (g_filter_cur + 1) % FILTER_CNT;
    memset(g_filter_hist, 0, sizeof(g_filter_hist));
}

#endif /* FILTER_USE_CMSIS */
