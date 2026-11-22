#ifndef __CS42L51_H
#define __CS42L51_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CS42L51_OK    0
#define CS42L51_ERROR (-1)

int cs42l51_init(void);

int cs42l51_start(void);

int cs42l51_stop(void);

/* Step is 0.5 dB */
int cs42l51_set_volume(int16_t half_db);

int cs42l51_set_mute(uint8_t mute);

#ifdef __cplusplus
}
#endif

#endif /* __CS42L51_H */
