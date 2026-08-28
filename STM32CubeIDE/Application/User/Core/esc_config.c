/*
 * Bit 0 = Motor 1
 * Bit 1 = Motor 2
 * Bit 2 = Motor 3
 * Bit 3 = Motor 4
 *
 * telematry_mask = 0x01; => Motor 1
 *
 * */

#include "esc_config.h"
#include "dshot.h"

#define ESC_CMD_SAVE_SETTINGS 12U
#define ESC_CMD_DIRECTION_NORMAL 20U
#define ESC_CMD_DIRECTION_REVERSED 21U
#define ESC_CMD_REPEAT_AMOUNT 10U
#define ESC_CMD_DSHOT_TIMEOUT 10U
#define ESC_CMD_SAVE_WAIT 50U


HAL_StatusTypeDef ESC_SetDirection(
		esc_channel_t channel,
		esc_direction_t direction,
		bool save_permanently)
{

}

static HAL_StatusTypeDef ESC_Wait(uint32_t timeout){
	uint32_t start = HAL_GetTick();

	while(DShot_IsBusy())
		if((HAL_GetTick() - start) >= timeout)
			return HAL_TIMEOUT;

	return HAL_OK;
}

// For one ESC Channel
static HAL_StatusTypeDef ESC_SendCMDRepeat(
		uint8_t amount,
		esc_channel_t channel,
		uint16_t cmd
		)
{
	uint16_t values[DSHOT_MOTOR_COUNT] = {
			0U, 0U, 0U, 0U
	};

	if((uint16_t) channel >= DSHOT_MOTOR_COUNT)
	{
		return HAL_ERROR;
	}

	// DShot-Values 0-3 in our case
	// values[0] is ESC 1
	values[(uint32_t) channel] = cmd;

	// 1 << 2 == 00000001 << 2 == 00000100 == 3
	// channel is index and result is esc
	uint8_t t_mask = (1U << channel);

	for(uint32_t repeat = 0U;
			repeat < ESC_CMD_REPEAT_AMOUNT;
			repeat++)
	{
		HAL_StatusTypeDef status = ESC_Wait(ESC_CMD_DSHOT_TIMEOUT);

		if(status != HAL_OK)
			return HAL_ERR
	}



}
