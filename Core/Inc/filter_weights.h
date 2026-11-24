#ifndef __FILTER_WEIGHTS_H
#define __FILTER_WEIGHTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math_types.h"

#define FILTER_WEIGHTS_LEN 161
#define FILTER_CNT 2

extern const float64_t g_filter_weights[FILTER_CNT][FILTER_WEIGHTS_LEN];

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_WEIGHTS_H */
