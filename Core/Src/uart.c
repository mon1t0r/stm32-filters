#include "uart.h"

#include "stm32h7xx_hal_def.h"
#include "mem_attr.h"

#include <string.h>

#define UART_TX_TIMEOUT_MS 100

/* Must be power of 2 */
#define UART_TX_BUF_SIZE 2048
#define UART_TX_BUF_MASK (UART_TX_BUF_SIZE - 1)

extern UART_HandleTypeDef huart1;

static uint8_t g_uart_buf[UART_TX_BUF_SIZE] __ATTR_RAM_D1;
static volatile uint32_t g_uart_start;
static volatile uint32_t g_uart_end;
/* Length during current transmission */
static volatile uint32_t g_uart_tx_len;
static volatile uint8_t g_uart_is_busy;

volatile uint32_t g_uart_drop_cnt = 0;

/* Critical section, only call when interrupts are disabled */
static void uart_kick(void)
{
    HAL_StatusTypeDef res;
    uint32_t len;

    /* If is transmitting or no data is present */
    if(g_uart_is_busy || g_uart_end == g_uart_start) {
        /* Try transmitting at the next kick call */
        return;
    }

    if(g_uart_end > g_uart_start) {
        len = g_uart_end - g_uart_start;
    } else {
        /* Wrap around */
        len = UART_TX_BUF_SIZE - g_uart_start;
    }

    res = HAL_UART_Transmit_DMA(
        &huart1,
        &g_uart_buf[g_uart_start],
        (uint16_t) len
    );
    if(res == HAL_OK) {
        /* Transmission started */
        g_uart_is_busy = 1;
        g_uart_tx_len = len;
    }

    /* Try transmitting at the next kick call */
}

static HAL_StatusTypeDef
uart_queue(const uint8_t *data_a, uint32_t data_a_sz,
           const uint8_t *data_b, uint32_t data_b_sz)
{
    uint32_t i;
    uint32_t primask;
    uint32_t buf_used;
    uint32_t buf_free;
    
    /* Backup primask (overriden by __disable_irq()) */
    primask = __get_PRIMASK();
    __disable_irq();
    /* Critical section start */

    buf_used = (g_uart_end - g_uart_start) & UART_TX_BUF_MASK;
    buf_free = UART_TX_BUF_SIZE - 1 - buf_used;

    if((data_a_sz + data_b_sz) > buf_free) {
        /* Unable to fit data into buffer free space */
        g_uart_drop_cnt++;

        /* Critical section end */
        __set_PRIMASK(primask);
        return HAL_BUSY;
    }

    /* Copy data A */
    for(i = 0; i < data_a_sz; i++) {
        g_uart_buf[g_uart_end] = data_a[i];
        g_uart_end = (g_uart_end + 1) & UART_TX_BUF_MASK;
    }

    /* Copy data B */
    for(i = 0; i < data_b_sz; i++) {
        g_uart_buf[g_uart_end] = data_b[i];
        g_uart_end = (g_uart_end + 1) & UART_TX_BUF_MASK;
    }

    /* Kick transmission */
    uart_kick();

    /* Critical section end */
    __set_PRIMASK(primask);
    return HAL_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t primask;

    if(huart->Instance != USART1) {
        return;
    }

    /* Backup primask (overriden by __disable_irq()) */
    primask = __get_PRIMASK();
    __disable_irq();

    /* Add transmitted amount */
    g_uart_start = (g_uart_start + g_uart_tx_len) & UART_TX_BUF_MASK;

    /* Reset transmission state */
    g_uart_tx_len = 0;
    g_uart_is_busy = 0;

    uart_kick();

    __set_PRIMASK(primask);
}

void uart_init(void)
{
    /* Zero out buffers */
    memset(g_uart_buf, 0, sizeof(g_uart_buf));
}

HAL_StatusTypeDef uart_write_u8(uint8_t v)
{
    return uart_queue(
        &v, sizeof(v),
        NULL, 0
    );
}

HAL_StatusTypeDef uart_write_u16(uint16_t v)
{
    return uart_queue(
        (const uint8_t *) &v, sizeof(v),
        NULL, 0
    );
}
