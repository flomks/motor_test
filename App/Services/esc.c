//
// Created by flori on 29.08.2026.
//

#include "esc.h"

#include "dshot.h"
#include "stm32h7xx_hal_def.h"


HAL_StatusTypeDef ESC_Init(void)
{
    static const uint16_t stop_vales[DSHOT_MOTOR_COUNT] = { 0U };

    HAL_StatusTypeDef status = DShot_Init();

    if (status != HAL_OK)
        return status;

    const uint32_t startup_begin = HAL_GetTick();

    do {
        return HAL_OK;
    }
    return HAL_OK;
}