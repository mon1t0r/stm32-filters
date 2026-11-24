#ifndef __FILTER_H
#define __FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FILTER_OK    0
#define FILTER_ERROR (-1)

int filter_init(void);

void filter_process(const int16_t *in, int16_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_H */
