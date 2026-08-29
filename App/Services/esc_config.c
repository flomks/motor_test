/**
 * @file esc_config.c
 * @brief DShot-based configuration service for four ESC channels.
 *
 * @details
 * Sends repeated DShot configuration commands with the telemetry bit set.
 * Non-selected channels receive motor-stop frames during configuration.
 */

#include "esc_config.h"
#include "dshot.h"

/** @brief DShot command that stores the current ESC settings. */
#define ESC_CMD_SAVE_SETTINGS 12U


/** @brief DShot command that selects normal motor direction. */
#define ESC_CMD_DIRECTION_NORMAL 20U

/** @brief DShot command that selects reversed motor direction. */
#define ESC_CMD_DIRECTION_REVERSED 21U

/** @brief Number of transmissions per configuration command. */
#define ESC_CMD_REPEAT_AMOUNT 10U

/** @brief Maximum wait for one DShot DMA transfer, in milliseconds. */
#define ESC_CMD_DSHOT_TIMEOUT_MS 10U

/** @brief Delay between repeated DShot commands, in milliseconds. */
#define ESC_CMD_REPEAT_INTERVAL_MS 1U

/** @brief Required wait after saving settings, in milliseconds. */
#define ESC_CMD_SAVE_WAIT_MS 50U

/** @brief Delay between consecutive configuration commands, in milliseconds. */
#define ESC_TIME_BETWEEN_CMD_MS 10U


/**
 * @brief Waits until the active DShot transfer finishes or times out.
 *
 * @param[in] timeout Maximum wait time in milliseconds.
 * @retval HAL_OK No DShot transfer is active.
 * @retval HAL_TIMEOUT The transfer did not finish before the timeout.
 */
static HAL_StatusTypeDef ESC_Wait(const uint32_t timeout){
	const uint32_t start = HAL_GetTick();

	while(DShot_IsBusy())
		if((HAL_GetTick() - start) >= timeout)
			return HAL_TIMEOUT;

	return HAL_OK;
}


/**
 * @brief Sends one DShot configuration command repeatedly to one ESC.
 *
 * @param[in] channel Target ESC channel.
 * @param[in] cmd DShot command value to transmit.
 * @retval HAL_OK All command repetitions completed successfully.
 * @retval HAL_ERROR The channel is invalid or transmission setup failed.
 * @retval HAL_BUSY The DShot driver is unavailable.
 * @retval HAL_TIMEOUT A DShot transfer did not finish in time.
 * @note The selected channel requests telemetry; all others receive stop.
 */
static HAL_StatusTypeDef ESC_SendCMDRepeat(
		const esc_channel_t channel,
		const uint16_t cmd
		)
{
	uint16_t values[DSHOT_MOTOR_COUNT] = {
			0U, 0U, 0U, 0U
	};

	if((uint16_t) channel >= DSHOT_MOTOR_COUNT)
	{
		return HAL_ERROR;
	}

	/* Array index 0 addresses ESC channel 1. */
	values[(uint32_t) channel] = cmd;

	/* Bit n selects channel n; for channel 2, 1U << 2U equals 0x04. */
	const uint8_t t_mask = (1U << channel);

	for(uint32_t repeat = 0U;
			repeat < ESC_CMD_REPEAT_AMOUNT;
			repeat++)
	{
		HAL_StatusTypeDef status = ESC_Wait(ESC_CMD_DSHOT_TIMEOUT_MS);

		if(status != HAL_OK)
			return status;

		status = DShot_Send(values, t_mask);

		if (status != HAL_OK)
			return status;

		status = ESC_Wait(ESC_CMD_DSHOT_TIMEOUT_MS);

		if(status != HAL_OK)
			return status;

		if ((repeat + 1U) < ESC_CMD_REPEAT_AMOUNT)
			HAL_Delay(ESC_CMD_REPEAT_INTERVAL_MS);
	}
	return HAL_OK;
}


/**
 * @brief Configures the rotation direction of one ESC.
 *
 * @param[in] channel ESC channel to configure.
 * @param[in] direction Normal or reversed rotation direction.
 * @param[in] save_permanently Whether to store the setting persistently.
 * @retval HAL_OK The direction command completed successfully.
 * @retval HAL_ERROR A parameter is invalid or transmission setup failed.
 * @retval HAL_BUSY The DShot driver is unavailable.
 * @retval HAL_TIMEOUT A DShot transfer did not finish in time.
 * @pre DShot must be initialized and every motor must be stopped.
 */
HAL_StatusTypeDef	ESC_SetDirection(
		const esc_channel_t channel,
		const esc_direction_t direction,
		const bool save_permanently)
{
	uint16_t cmd;

	/* Reject channels outside the configured motor range. */
	if ((uint32_t)channel >= DSHOT_MOTOR_COUNT)
		return HAL_ERROR;

	/* Select the command corresponding to the requested direction. */
	switch (direction) {
		case ESC_DIRECTION_REVERSED:
			cmd = ESC_CMD_DIRECTION_REVERSED;
			break;

		case ESC_DIRECTION_NORMAL:
			cmd = ESC_CMD_DIRECTION_NORMAL;
			break;

		/* Reject unsupported enum values. */
		default:
			return HAL_ERROR;
	}

	/* Send the direction command repeatedly as required by DShot. */
	HAL_StatusTypeDef status = ESC_SendCMDRepeat(channel, cmd);
	if (status != HAL_OK)
		return status;

	if (!save_permanently)
		return HAL_OK;

	/* Store the new direction in the ESC's non-volatile memory. */
	HAL_Delay(ESC_TIME_BETWEEN_CMD_MS);

	status = ESC_SendCMDRepeat(channel, ESC_CMD_SAVE_SETTINGS);

	if (status != HAL_OK)
		return status;

	HAL_Delay(ESC_CMD_SAVE_WAIT_MS);
	return HAL_OK;
}


/**
 * @brief Plays the beacon tone on one ESC.
 *
 * @details The ESC drives the motor windings at an audible frequency, so the
 *          motor itself acts as the speaker. The shaft does not turn.
 *
 * @param[in] channel ESC channel that should emit the tone.
 * @param esc_cmd_beacon DShot defines five beacons as commands 1 to 5, each with a different
 *       pitch. Change this value to select another one.
 * @retval HAL_OK The beacon command completed successfully.
 * @retval HAL_ERROR The channel is invalid or transmission setup failed.
 * @retval HAL_BUSY The DShot driver is unavailable.
 * @retval HAL_TIMEOUT A DShot transfer did not finish in time.
 * @pre DShot must be initialized and every motor must be stopped.
 * @note The setting is not persistent, so no save command follows.
 * @note ESCs typically reject throttle for a short moment afterwards. That is
 *       expected behaviour and not an error.
 */
HAL_StatusTypeDef ESC_Beep(const esc_channel_t channel, uint8_t esc_cmd_beacon)
{
	/* Reject channels outside the configured motor range. */
	if ((uint32_t)channel >= DSHOT_MOTOR_COUNT)
		return HAL_ERROR;

	if ((esc_cmd_beacon < 1U) || (esc_cmd_beacon > 5U))
		return HAL_ERROR;

	return ESC_SendCMDRepeat(channel, esc_cmd_beacon);
}
