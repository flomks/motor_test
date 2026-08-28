/*
 * dshot.h
 *
 *  Created on: 26.08.2026
 *      Author: flo
 */

#ifndef APPLICATION_INC_DSHOT_H_
#define APPLICATION_INC_DSHOT_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define DSHOT_MOTOR_COUNT 4U

// STM32-HAL header
HAL_StatusTypeDef DShot_Init(void);

HAL_StatusTypeDef DShot_Send(
		const uint16_t values[DSHOT_MOTOR_COUNT],
		uint8_t t_mask);

typedef enum {
	DSHOT_TELEMETRY_NONE = 0U,
	DSHOT_TELEMETRY_MOTOR_1 = (1U << 0U),
	DSHOT_TELEMETRY_MOTOR_2 = (1U << 1U),
	DSHOT_TELEMETRY_MOTOR_3 = (1U << 2U),
	DSHOT_TELEMETRY_MOTOR_4 = (1U << 3U),
	DSHOT_TELEMETRY_MOTOR_ALL= 0x0FU,
} dshot_telemetry_mask_t;

bool DShot_IsBusy(void);

void DShot_TransferComplete(void);

void DShot_Abort(void);

#endif /* APPLICATION_INC_DSHOT_H_ */
