#ifdef FILTER_USE_CMSIS

#include "filter.h"

#include <string.h>
#include "filter_weights.h"
#include "uart.h"
#include "audio.h"

#include "arm_math.h"

/* Filter instance */
static arm_fir_instance_f64 g_filters[FILTER_CNT][AUDIO_CHANNELS];

/* Filter history (all channels) */
static float64_t g_filter_state[FILTER_CNT][AUDIO_CHANNELS][FILTER_WEIGHTS_LEN + AUDIO_FRAMES_PER_BLOCK - 1];

/* Filter input/output buffers (single channel) */
static float64_t g_filter_input[AUDIO_FRAMES_PER_BLOCK];
static float64_t g_filter_output[AUDIO_FRAMES_PER_BLOCK];

/* Current filter index */
static int g_filter_cur = 0;

int filter_init(void)
{
    uint32_t fil, ch;

    memset(g_filter_state, 0, sizeof(g_filter_state));

    for(fil = 0; fil < FILTER_CNT; fil++) {
        for(ch = 0; ch < AUDIO_CHANNELS; ch++) {
            arm_fir_init_f64(
                &g_filters[fil][ch],
                FILTER_WEIGHTS_LEN,
                &g_filter_weights[fil][0],
                &g_filter_state[fil][ch][0],
                AUDIO_FRAMES_PER_BLOCK
            );
        }
    }

    return FILTER_OK;
}

void filter_process(const int16_t *in, int16_t *out)
{
    uint32_t ch, f;

    /* Compute filter results for each channel in a block */
    for(ch = 0; ch < AUDIO_CHANNELS; ch++) {
        for(f = 0; f < AUDIO_FRAMES_PER_BLOCK; f++) {
            g_filter_input[f] = in[f * AUDIO_CHANNELS + ch];
        }

        arm_fir_f64(
            &g_filters[g_filter_cur][ch],
            g_filter_input,
            g_filter_output,
            AUDIO_FRAMES_PER_BLOCK
        );

        for(f = 0; f < AUDIO_FRAMES_PER_BLOCK; f++) {
            out[f * AUDIO_CHANNELS + ch] = g_filter_output[f];
        }
    }

    /* Write result to UART */
    for(f = 0; f < AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS; f++) {
        uart_write_u16(out[f]);
    }
}

void filter_next(void)
{
    g_filter_cur = (g_filter_cur + 1) % FILTER_CNT;
}

#endif /* FILTER_USE_CMSIS */
