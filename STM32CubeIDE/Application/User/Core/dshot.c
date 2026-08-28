/*
 * DShot logic
 *
 * 11 Bit value
 * 1 Bit telematry
 * 4 Bit checksum
 *
 * VVVVVVVVVVVTCCCC
 *
 */

/*
 *
 * Payload = value << 1
 * telematry = ggf. in bit 0
 * checksum = XOR aus den drei 4 bit blöcken des P
 *
 * package = payload << 4 + checksum
 *
 */

/*
 * TIM1 Clock = 240 MHz
 * ARR = 399
 * Bitdauer = 400 Ticks
 */

#include "dshot.h"
#include "tim.h"
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * TICKS
 */
#define DSHOT_BIT_0 150U
#define DSHOT_BIT_1 300U

#define DSHOT_FRAME_BITS 16U

// #define DSHOT_FRAME_BITS 16
#define DSHOT_GAP_SLOTS 2U
#define DSHOT_TOTAL_SLOTS (DSHOT_FRAME_BITS + DSHOT_GAP_SLOTS)
// #define DSHOT_DMA_WORD_COUNT 32
#define DSHOT_DMA_WORD_COUNT (DSHOT_TOTAL_SLOTS * DSHOT_MOTOR_COUNT)


static uint32_t dma_buffer[DSHOT_TOTAL_SLOTS][DSHOT_MOTOR_COUNT]; // 18 4

static volatile bool dshot_busy = false;


static uint16_t DShot_CreatePackage (uint16_t value, bool telemetry)
{
	//value = 0x07FFU; // 11111111111 in hex
	assert(value <= 0x07FFU);

	uint16_t payload = (uint16_t)(value << 1) | (uint16_t) telemetry;

	uint16_t c_sum =  (uint16_t) ((payload ^
									(payload >> 4) ^
									(payload >> 8)) & 0x0F);

	return  (uint16_t)((payload << 4) | c_sum);

}

static void DShot_EncoderFrame (uint16_t d, uint32_t buffer[DSHOT_FRAME_BITS])
{
    for(uint32_t index = 0U;
            index < DSHOT_FRAME_BITS;
            index++)
    {
        uint32_t pos = DSHOT_FRAME_BITS - 1U - index;

        uint16_t mask = (uint16_t) (1U << pos);

        buffer[index] = (d & mask) != 0U ? DSHOT_BIT_1 : DSHOT_BIT_0;
    }
}

static bool DShot_DMABuffer_Builder(
		const uint16_t values[DSHOT_MOTOR_COUNT],
		uint8_t t_mask)
{
	uint32_t encoded[DSHOT_FRAME_BITS];

	if(!values) return false;

	for(uint32_t motor = 0U;
			motor < DSHOT_MOTOR_COUNT;
			motor++)
	{
		if(values[motor]> 0x07FFU) return false;

		bool t = (t_mask & (1U << motor)) != 0U;


		uint16_t packet = DShot_CreatePackage(values[motor], t);

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

bool DShot_IsBusy(void){
	return dshot_busy;
}

HAL_StatusTypeDef DShot_Send(
		const uint16_t values[DSHOT_MOTOR_COUNT],
		uint8_t t_mask)
{
	if(dshot_busy)
		return HAL_BUSY;

	if(!DShot_DMABuffer_Builder(values, t_mask))
		return HAL_ERROR;

	dshot_busy = true;

	HAL_StatusTypeDef status = HAL_TIM_DMABurst_MultiWriteStart(
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














