#ifndef __FILTER_WEIGHTS_H
#define __FILTER_WEIGHTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math_types.h"

#if defined(FILTER_MODE_INT) && defined(FILTER_USE_CMSIS) && defined(ARM_MATH_LOOPUNROLL)
/* CMSIS - INT16 - LOOPUNROLL */

/* 1550's order - 1551 coeficients */
/* For integer version, number must be even - 1552, zero is added in the array */
#define FILTER_WEIGHTS_LEN 1552

#elif defined(FILTER_MODE_INT) && defined(FILTER_USE_CMSIS) && !defined(ARM_MATH_LOOPUNROLL)
/* CMSIS - INT16 - No LOOPUNROLL */

/* 330's order - 331 coeficients */
/* For integer version, number must be even - 332, zero is added in the array */
#define FILTER_WEIGHTS_LEN 332

#elif defined(FILTER_MODE_INT) && !defined(FILTER_USE_CMSIS)
/* CUSTOM - INT16 */

/* 180's order - 181 coeficients */
#define FILTER_WEIGHTS_LEN 181

#elif !defined(FILTER_MODE_INT) && defined(FILTER_USE_CMSIS)
/* CMSIS - FLOAT64 */

/* 240's order - 241 coeficients */
#define FILTER_WEIGHTS_LEN 241

#elif !defined(FILTER_MODE_INT) && !defined(FILTER_USE_CMSIS)
/* CUSTOM - FLOAT64 */

/* 190's order - 191 coeficients */
#define FILTER_WEIGHTS_LEN 191

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
