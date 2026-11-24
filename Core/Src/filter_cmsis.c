#define FILTER_USE_CMSIS /* TODO: Remove */

#ifdef FILTER_USE_CMSIS

#include "filter.h"

#include <string.h>
#include "filter_weights.h"
#include "uart.h"
#include "audio.h"

#include "arm_math.h"

/* Filter instance */
static arm_fir_instance_f64 g_filter[AUDIO_CHANNELS];

/* Filter history (all channels) */
static float64_t g_filter_state[(WEIGHTS_LEN + AUDIO_FRAMES_PER_BLOCK) * AUDIO_CHANNELS];

/* Filter input/output buffers (single channel) */
static float64_t g_filter_input[AUDIO_FRAMES_PER_BLOCK];
static float64_t g_filter_output[AUDIO_FRAMES_PER_BLOCK];

int filter_init(void)
{
    memset(g_filter_state, 0, sizeof(g_filter_state));

    for(uint32_t ch = 0; ch < AUDIO_CHANNELS; ch++) {
        arm_fir_init_f64(
            &g_filter[ch],
            WEIGHTS_LEN,
            g_weights,
            &g_filter_state[ch * (WEIGHTS_LEN + AUDIO_FRAMES_PER_BLOCK)],
            AUDIO_FRAMES_PER_BLOCK
        );
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
            &g_filter[ch],
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

#endif /* FILTER_USE_CMSIS */
