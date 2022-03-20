/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @author         : Matthias Kurz
  * @brief          : Illustrates intertask synchronization with semaphores.
  *                   Two producer tasks, in parallel, each increments a number
  *                   of a global struct and saves the number in the same
  *                   global struct inside an accompanying char array. Meanwhile,
  *                   a consumer task divides the values by half (if they are
  *                   odd) and updates the according char array as well.
  *                   A fourth task outputs the values and the char array via
  *                   the UART2. When not using semaphores the number values and
  *                   their accompanying char arrays diverge. This can be tested
  *                   by removing the "USE_SEM" define.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VALUE_STR_LENGTH 10
#define USE_SEM // Remove or comment to see what happens without semaphores
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for producerTask1 */
osThreadId_t producerTask1Handle;
const osThreadAttr_t producerTask1_attributes = {
  .name = "producerTask1",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for producerTask2 */
osThreadId_t producerTask2Handle;
const osThreadAttr_t producerTask2_attributes = {
  .name = "producerTask2",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for consumerTask */
osThreadId_t consumerTaskHandle;
const osThreadAttr_t consumerTask_attributes = {
  .name = "consumerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for watcherTask */
osThreadId_t watcherTaskHandle;
const osThreadAttr_t watcherTask_attributes = {
  .name = "watcherTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for t1BinarySem */
osSemaphoreId_t t1BinarySemHandle;
const osSemaphoreAttr_t t1BinarySem_attributes = {
  .name = "t1BinarySem"
};
/* Definitions for t2BinarySem */
osSemaphoreId_t t2BinarySemHandle;
const osSemaphoreAttr_t t2BinarySem_attributes = {
  .name = "t2BinarySem"
};
/* USER CODE BEGIN PV */

/* The global struct all tasks access */
struct _data {
  int t1value;
  char t1str[VALUE_STR_LENGTH];
  int t2value;
  char t2str[VALUE_STR_LENGTH];
} data;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void task1_produce_data(void *argument);
void task2_produce_data(void *argument);
void consume_data(void *argument);
void watch_data(void *argument);

/* USER CODE BEGIN PFP */
void increase_values(int *value, char* valueStr, osSemaphoreId_t semaphoreHandle);
void half_values(int *value, char* valueStr, osSemaphoreId_t semaphoreHandle);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Print given character on UART 2. Translate '\n' to "\r\n" on the fly. */
int __io_putchar(int ch)
{
  int ret;
  while ((ret=HAL_UART_GetState(&huart2)) != HAL_UART_STATE_READY)
    ;

  if (ch == '\n')
  {
    static uint8_t buf[2] = { '\r', '\n' };
    HAL_UART_Transmit_IT(&huart2, buf, sizeof(buf));
  }
  else
  {
    static char buf;
    buf = ch;
    HAL_UART_Transmit_IT(&huart2, (uint8_t *)&buf, 1);
  }
  return ch;
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of t1BinarySem */
  t1BinarySemHandle = osSemaphoreNew(1, 1, &t1BinarySem_attributes);

  /* creation of t2BinarySem */
  t2BinarySemHandle = osSemaphoreNew(1, 1, &t2BinarySem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of producerTask1 */
  producerTask1Handle = osThreadNew(task1_produce_data, NULL, &producerTask1_attributes);

  /* creation of producerTask2 */
  producerTask2Handle = osThreadNew(task2_produce_data, NULL, &producerTask2_attributes);

  /* creation of consumerTask */
  consumerTaskHandle = osThreadNew(consume_data, NULL, &consumerTask_attributes);

  /* creation of watcherTask */
  watcherTaskHandle = osThreadNew(watch_data, NULL, &watcherTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/**
  * @brief  Increases the value of the given int pointer. Also saves that value in the given char array (string).
  * @param  *value: Pointer to the Integer that should incremented.
  * @param  *valueStr: Pointer to the char array (string) in which the increased int value should be saved to (as string of course).
  * @param  semaphoreHandle: The semaphore used to protect the critical code that increases the int number and saves it in the string.
  * @retval None
  */
void increase_values(int *value, char* valueStr, osSemaphoreId_t semaphoreHandle)
{
  #ifdef USE_SEM
    if(osSemaphoreAcquire(semaphoreHandle, osWaitForever) == osOK)
    {
  #endif
      (*value)++; // increment operator does NOT have atomic behavior (read value, then increment it, then write it back into memory)
      snprintf(valueStr, VALUE_STR_LENGTH, "%d", *value);
  #ifdef USE_SEM
      osSemaphoreRelease(semaphoreHandle);
    }
  #endif
}

/**
  * @brief  Divides the value of the given int pointer by half. Also saves that value in the given char array (string).
  * @param  *value: Pointer to the Integer that should be divided by half.
  * @param  *valueStr: Pointer to the char array (string) in which the divided int value should be saved to (as string of course).
  * @param  semaphoreHandle: The semaphore used to protect the critical code that divides the int number and saves it in the string.
  * @retval None
  */
void half_values(int *value, char* valueStr, osSemaphoreId_t semaphoreHandle)
{
  #ifdef USE_SEM
    if(osSemaphoreAcquire(semaphoreHandle, osWaitForever) == osOK)
    {
  #endif
      if((*value) % 2 == 0)
      {
        (*value) /= 2; // Not atomic
        snprintf(valueStr, VALUE_STR_LENGTH, "%d", *value);
      }
  #ifdef USE_SEM
      osSemaphoreRelease(semaphoreHandle);
    }
  #endif
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_task1_produce_data */
/**
  * @brief  The producerTask1 thread periodically increases the t1 int value and its accompanying string of the global struct.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_task1_produce_data */
void task1_produce_data(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    increase_values(&data.t1value, data.t1str, t1BinarySemHandle);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_task2_produce_data */
/**
* @brief  The producerTask2 thread periodically increases the t2 int value and its accompanying string of the global struct.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_task2_produce_data */
void task2_produce_data(void *argument)
{
  /* USER CODE BEGIN task2_produce_data */
  /* Infinite loop */
  for(;;)
  {
    increase_values(&data.t2value, data.t2str, t2BinarySemHandle);
  }
  /* USER CODE END task2_produce_data */
}

/* USER CODE BEGIN Header_consume_data */
/**
* @brief The consumerTask thread divides the t1* and t2* values of the global struct by half.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_consume_data */
void consume_data(void *argument)
{
  /* USER CODE BEGIN consume_data */
  /* Infinite loop */
  for(;;)
  {
    half_values(&data.t1value, data.t1str, t1BinarySemHandle);
    half_values(&data.t2value, data.t2str, t2BinarySemHandle);
  }
  /* USER CODE END consume_data */
}

/* USER CODE BEGIN Header_watch_data */
/**
* @brief The watcherTask thread outputs all values via the UART2.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_watch_data */
void watch_data(void *argument)
{
  /* USER CODE BEGIN watch_data */
  /* Infinite loop */
  for(;;)
  {
    printf("\n\n\n\n");
    printf("Each value in its own printf statement:\n");
    printf("---------------------------------------\n");
    #ifdef USE_SEM
      if(osSemaphoreAcquire(t1BinarySemHandle, osWaitForever) == osOK)
      {
    #endif
        printf("t1value: %d\n", data.t1value);
        printf("t1str: %s\n", data.t1str);
    #ifdef USE_SEM
        osSemaphoreRelease(t1BinarySemHandle);
      }
    #endif
    #ifdef USE_SEM
      if(osSemaphoreAcquire(t2BinarySemHandle, osWaitForever) == osOK)
      {
    #endif
        printf("t2value: %d\n", data.t2value);
        printf("t2str: %s\n", data.t2str);
    #ifdef USE_SEM
        osSemaphoreRelease(t2BinarySemHandle);
      }
    #endif

    printf("\nAll values in one single printf statement:\n");
    printf("------------------------------------------\n");
    #ifdef USE_SEM
      if(osSemaphoreAcquire(t1BinarySemHandle, osWaitForever) == osOK)
      {
        // Nest the acquire statements instead of putting them all inside one single "if".
        // If we would put them in one single "if" it could happen we acquire the first but not the second one,
        // meaning that in case the "if" branch would not be executed we would not release the first semaphore anymore!
        if(osSemaphoreAcquire(t2BinarySemHandle, osWaitForever) == osOK)
        {
    #endif
          printf("t1value: %d, t1str: %s, t2value: %d, t2str: %s\n", data.t1value, data.t1str, data.t2value, data.t2str);
    #ifdef USE_SEM
          osSemaphoreRelease(t2BinarySemHandle); // Release the inner semaphore
        }
        osSemaphoreRelease(t1BinarySemHandle); // Release the outer semaphore
      }
    #endif

    osDelay(3000); // Enough (screen) time to watch what's going on ;)
  }
  /* USER CODE END watch_data */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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

#ifdef  USE_FULL_ASSERT
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

