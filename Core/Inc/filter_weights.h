#ifndef __FILTER_WEIGHTS_H
#define __FILTER_WEIGHTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math_types.h"

/* Filters are 160's order - number of coeficients is 161 */
/* For integer version, number must be even - 162, zero is added in the array */

#ifdef FILTER_MODE_INT
#define FILTER_WEIGHTS_LEN 162
#else
#define FILTER_WEIGHTS_LEN 161
#endif

#define FILTER_CNT 4

#ifdef FILTER_MODE_INT
typedef q15_t                fil_t;
#else
typedef float64_t            fil_t;
#endif

extern const fil_t g_filter_weights[FILTER_CNT][FILTER_WEIGHTS_LEN];

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_WEIGHTS_H */
