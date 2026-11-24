#include "button.h"

#include "main.h"
#include "filter.h"

#define BUTTON_DEBOUNCE_MS 200

/* Last press time, ms */
static uint32_t g_last_press = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now;

    if(GPIO_Pin != WAKEUP_Pin) {
        return;
    }

    /* Debounce */
    now = HAL_GetTick();
    if((now - g_last_press) < BUTTON_DEBOUNCE_MS) {
        return;
    }
    g_last_press = now;
    
    filter_next();
}
