/**
 * @file dshot.c
 * @brief DShot600 driver for four ESC channels using TIM1 DMA burst mode.
 *
 * @details
 * Creates 16-bit DShot packets, converts their bits into timer compare values
 * and transmits four motor channels through a shared DMA buffer.
 *
 * Packet format: 11 value bits, one telemetry bit and four checksum bits.
 * @code
 * VVVVVVVVVVVTCCCC
 * @endcode
 *
 * TIM1 configuration:
 * - Timer clock: 240 MHz
 * - Prescaler: 0
 * - Auto-reload value: 399
 * - Timer ticks per DShot bit: 400
 * - Resulting bitrate: 600 kbit/s
 */

#include "dshot.h"
#include "tim.h"
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>


/** @brief Compare value for a logical zero (37.5% high time). */
#define DSHOT_BIT_0 150U

/** @brief Compare value for a logical one (75% high time). */
#define DSHOT_BIT_1 300U

/** @brief Number of bits in one DShot packet. */
#define DSHOT_FRAME_BITS 16U

/** @brief Number of low-level slots appended after each packet. */
#define DSHOT_GAP_SLOTS 2U

/** @brief Total number of DMA slots per motor and transmission. */
#define DSHOT_TOTAL_SLOTS (DSHOT_FRAME_BITS + DSHOT_GAP_SLOTS)

/** @brief Total number of 32-bit DMA words for all motor channels. */
#define DSHOT_DMA_WORD_COUNT (DSHOT_TOTAL_SLOTS * DSHOT_MOTOR_COUNT)

/**
 * @brief Shared DMA buffer indexed by timer slot and motor channel.
 *
 * @note The first dimension selects the slot; the second selects the motor.
 */
static uint32_t dma_buffer[DSHOT_TOTAL_SLOTS][DSHOT_MOTOR_COUNT]; // 18 x 4

/** @brief Indicates whether a DShot DMA transfer is active. */
static volatile bool dshot_busy = false;


/**
 * @brief Creates a complete 16-bit DShot packet.
 *
 * @param[in] value DShot value or command in the range 0 to 2047.
 * @param[in] telemetry Whether to set the telemetry request bit.
 * @return Encoded 16-bit DShot packet including its checksum.
 * @pre value must not exceed 0x07FF.
 */
static uint16_t DShot_CreatePackage (const uint16_t value, const bool telemetry)
{
	//value = 0x07FFU; // 11111111111 in hex
	assert(value <= 0x07FFU);

	const uint16_t payload = (uint16_t)(value << 1) | (uint16_t) telemetry;

	const uint16_t c_sum =  (uint16_t) ((payload ^
									(payload >> 4) ^
									(payload >> 8)) & 0x0F);

	return  (uint16_t)((payload << 4) | c_sum);

}


/**
 * @brief Converts a DShot packet into PWM compare values, MSB first.
 *
 * @param[in] d Complete 16-bit DShot packet.
 * @param[out] buffer Destination for DSHOT_FRAME_BITS compare values.
 */
static void DShot_EncoderFrame (const uint16_t d, uint32_t buffer[DSHOT_FRAME_BITS])
{
    for(uint32_t index = 0U;
            index < DSHOT_FRAME_BITS;
            index++)
    {
        const uint32_t pos = DSHOT_FRAME_BITS - 1U - index;

        const uint16_t mask = (uint16_t) (1U << pos);

        buffer[index] = (d & mask) != 0U ? DSHOT_BIT_1 : DSHOT_BIT_0;
    }
}


/**
 * @brief Builds the four-channel DMA buffer for one DShot transmission.
 *
 * @param[in] values One DShot value for each motor channel.
 * @param[in] t_mask Telemetry mask; bit n selects motor channel n.
 * @retval true The DMA buffer was generated successfully.
 * @retval false values is NULL or contains an invalid DShot value.
 */
static bool DShot_DMABuffer_Builder(
		const uint16_t values[DSHOT_MOTOR_COUNT],
		const uint8_t t_mask)
{
	uint32_t encoded[DSHOT_FRAME_BITS];

	if(!values) return false;

	for(uint32_t motor = 0U;
			motor < DSHOT_MOTOR_COUNT;
			motor++)
	{
		if(values[motor]> 0x07FFU) return false;

		const bool t = (t_mask & (1U << motor)) != 0U;


		const uint16_t packet = DShot_CreatePackage(values[motor], t);

		DShot_EncoderFrame(packet, encoded);


		for(uint32_t index = 0U;
				index < DSHOT_FRAME_BITS;
				index++)
		{
			dma_buffer[index][motor] = encoded[index];
		}
	}

	for(uint32_t slot = DSHOT_FRAME_BITS;
			slot < DSHOT_TOTAL_SLOTS;
			slot++)
	{
		for(uint32_t motor = 0U;
				motor < DSHOT_MOTOR_COUNT;
				motor++)
		{
			dma_buffer[slot][motor] = 0U;
		}
	}

	return true;
}


/**
 * @brief Initializes the DShot output and starts all four PWM channels low.
 *
 * @retval HAL_OK All PWM channels were started successfully.
 * @retval HAL_ERROR At least one PWM channel could not be started.
 * @pre MX_DMA_Init() and MX_TIM1_Init() must have completed successfully.
 */
HAL_StatusTypeDef DShot_Init(void){
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0U);

	__HAL_TIM_SET_COUNTER(&htim1, 0U);

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK)
    {
        return HAL_ERROR;
    }

    dshot_busy = false;

    return HAL_OK;
}


/**
 * @brief Reports whether a DShot DMA transfer is active.
 *
 * @retval true A transmission is currently active.
 * @retval false The driver is ready for another transmission.
 */
bool DShot_IsBusy(void){
	return dshot_busy;
}


/**
 * @brief Starts an asynchronous four-channel DShot transmission.
 *
 * @param[in] values One DShot value for each motor channel.
 * @param[in] t_mask Telemetry mask; bit n selects motor channel n.
 * @retval HAL_OK The DMA transfer was started successfully.
 * @retval HAL_BUSY A previous transfer is still active.
 * @retval HAL_ERROR The input is invalid or DMA could not be started.
 * @pre DShot_Init() must have completed successfully.
 */
HAL_StatusTypeDef DShot_Send(
		const uint16_t values[DSHOT_MOTOR_COUNT],
		const uint8_t t_mask)
{
	if(dshot_busy)
		return HAL_BUSY;

	if(!DShot_DMABuffer_Builder(values, t_mask))
		return HAL_ERROR;

	dshot_busy = true;

	const HAL_StatusTypeDef status = HAL_TIM_DMABurst_MultiWriteStart(
			&htim1,
			TIM_DMABASE_CCR1,
			TIM_DMA_UPDATE,
			&dma_buffer[0][0],
			TIM_DMABURSTLENGTH_4TRANSFERS,
			DSHOT_DMA_WORD_COUNT);

	if(status != HAL_OK)
		dshot_busy = false;

	return status;
}


/**
 * @brief Finalizes a DShot transfer and drives all outputs low.
 *
 * @note Call this function from the TIM1 DMA transfer-completion callback.
 */
void DShot_TransferComplete(void){
	if(!dshot_busy)
		return;

	HAL_TIM_DMABurst_WriteStop(
			&htim1,
			TIM_DMA_UPDATE);

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0U);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0U);

	dshot_busy = false;
}
