#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#include <stdint.h>

/* UART error count */
/* Used while debugging */
extern volatile uint32_t g_uart_drop_cnt;

void uart_init(void);

HAL_StatusTypeDef uart_write_u8(uint8_t v);

HAL_StatusTypeDef uart_write_u16(uint16_t v);

uint32_t uart_write_mem(const void *buf, uint32_t len);

HAL_StatusTypeDef uart_print(const char *str);

HAL_StatusTypeDef uart_println(const char *str);

HAL_StatusTypeDef uart_flush(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
