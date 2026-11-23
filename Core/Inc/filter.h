#ifndef __FILTER_H
#define __FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void filter_init(void);

void filter_process(const int16_t *in, int16_t *out, uint32_t frames);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_H */
