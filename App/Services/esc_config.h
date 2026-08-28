/*
 * esc_config.h
 *
 *  Created on: 26.08.2026
 *      Author: flori
 */

#ifndef APPLICATION_INC_ESC_CONFIG_H_
#define APPLICATION_INC_ESC_CONFIG_H_

#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	ESC_CHANNEL_1 = 0U,
	ESC_CHANNEL_2,
	ESC_CHANNEL_3,
	ESC_CHANNEL_4
} esc_channel_t;

typedef enum
{
	ESC_DIRECTION_NORMAL = 0U,
	ESC_DIRECTION_REVERSED
} esc_direction_t;

HAL_StatusTypeDef ESC_SetDirection(
		esc_channel_t channel,
		esc_direction_t direction,
		bool save_permanently);

#endif /* APPLICATION_INC_ESC_CONFIG_H_ */
