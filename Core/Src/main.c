/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ESC_RAMP_START_US          1000U
#define ESC_RAMP_END_US            1200U
#define ESC_RAMP_STEP_US           10U
#define ESC_RAMP_STEP_DELAY_MS     100U
#define ESC_MAX_HOLD_TIME_MS       2000U
#define ESC_ARM_TIME_MS            3000U
#define ESC_STOP_TIME_MS           3000U

#if (ESC_RAMP_STEP_US == 0U)
#error "ESC_RAMP_STEP_US must be greater than zero"
#endif

#if (ESC_RAMP_START_US < 1000U) || (ESC_RAMP_END_US > 2000U) || \
    (ESC_RAMP_END_US < ESC_RAMP_START_US)
#error "ESC ramp must stay within 1000..2000 us and end at or above start"
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

static void ESC_SetTestMotorPulse(uint32_t pulse_us);
static HAL_StatusTypeDef ESC_StartTestMotor(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void ESC_SetTestMotorPulse(uint32_t pulse_us)
{
  /* __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse_us); */
  /* __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pulse_us); */
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pulse_us);
  /* __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse_us); */
}

static HAL_StatusTypeDef ESC_StartTestMotor(void)
{
  /* HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); */
  /* HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); */

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); */

  return HAL_OK;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  /* Arm only Motor 3 (TIM1_CH3 / PA10). Test without propellers fitted. */
  ESC_SetTestMotorPulse(ESC_RAMP_START_US);

  if (ESC_StartTestMotor() != HAL_OK)
  {
    Error_Handler();
  }

  HAL_Delay(ESC_ARM_TIME_MS);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Ramp Motor 3 from the start value up to the end value. */
    for (uint32_t pulse_us = ESC_RAMP_START_US;
         pulse_us < ESC_RAMP_END_US;
         pulse_us += ESC_RAMP_STEP_US)
    {
      ESC_SetTestMotorPulse(pulse_us);
      HAL_Delay(ESC_RAMP_STEP_DELAY_MS);
    }

    /* Hold the configured maximum value. */
    ESC_SetTestMotorPulse(ESC_RAMP_END_US);
    HAL_Delay(ESC_MAX_HOLD_TIME_MS);

    /* Ramp Motor 3 back down without unsigned-integer underflow. */
    uint32_t pulse_us = ESC_RAMP_END_US;
    while (pulse_us > ESC_RAMP_START_US)
    {
      if ((pulse_us - ESC_RAMP_START_US) > ESC_RAMP_STEP_US)
      {
        pulse_us -= ESC_RAMP_STEP_US;
      }
      else
      {
        pulse_us = ESC_RAMP_START_US;
      }

      ESC_SetTestMotorPulse(pulse_us);
      HAL_Delay(ESC_RAMP_STEP_DELAY_MS);
    }

    /* Stay stopped before starting the next ramp. */
    HAL_Delay(ESC_STOP_TIME_MS);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
