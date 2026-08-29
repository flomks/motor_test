//
// Created by flori on 29.08.2026.
//

#include "esc.h"

#include "dshot.h"
#include "stm32h7xx_hal_def.h"

#define ESC_STARTUP_TIME_MS 3000U
#define ESC_DSHOT_TIMEOUT_MS 10U
#define ESC_FRAME_INTERVAL_MS 1U

/**
 *
 * @return
 */
HAL_StatusTypeDef ESC_WaitForDShot(void) {
    const uint32_t start = HAL_GetTick();

    while (DShot_IsBusy())
    {
        if ((HAL_GetTick() - start) >= ESC_DSHOT_TIMEOUT_MS) {
            DShot_Abort();
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}


/**
 *
 * @return
 */
HAL_StatusTypeDef ESC_Init(void)
{
    static const uint16_t stop_vales[DSHOT_MOTOR_COUNT] = { 0U };

    HAL_StatusTypeDef status = DShot_Init();

    if (status != HAL_OK)
        return status;

    const uint32_t startup_begin = HAL_GetTick();

    do {
        status = DShot_Send(stop_vales, 0U);
        if (status != HAL_OK)
            return status;

        status = ESC_WaitForDShot();
        if (status != HAL_OK)
            return status;

        HAL_Delay(ESC_FRAME_INTERVAL_MS);

    } while (HAL_GetTick() - startup_begin < ESC_STARTUP_TIME_MS);

    return HAL_OK;
}