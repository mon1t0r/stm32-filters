#include "filter.h"

#include <string.h>
#include "uart.h"
#include "audio.h"

void filter_init(void)
{

}

void filter_process(const int16_t *in, int16_t *out, uint32_t frames)
{
    memcpy(out, in, frames * AUDIO_CHANNELS * sizeof(*out));
}
