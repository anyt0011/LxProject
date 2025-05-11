/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "stm32f411_task_key.h"
#include "stm32f411_task_led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for key_task */
osThreadId_t key_taskHandle;
const osThreadAttr_t key_task_attributes = {
  .name = "key_task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for led_task */
osThreadId_t led_taskHandle;
const osThreadAttr_t led_task_attributes = {
  .name = "led_task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LedTimer */
osTimerId_t LedTimerHandle;
const osTimerAttr_t LedTimer_attributes = {
  .name = "LedTimer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* Definitions for led_task */
osThreadId_t ota_taskHandle;
const osThreadAttr_t ota_task_attributes = {
  .name = "ota_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
SemaphoreHandle_t binarySemaphore;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void key_task_runnable(void *argument);
void led_task_runnable(void *argument);
void LedCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  binarySemaphore = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of LedTimer */
  LedTimerHandle = osTimerNew(LedCallback, osTimerOnce, NULL, &LedTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of key_task */
  key_taskHandle = osThreadNew(key_task_runnable, NULL, &key_task_attributes);

  /* creation of led_task */
  led_taskHandle = osThreadNew(led_task_runnable, NULL, &led_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  ota_taskHandle = osThreadNew(ota_task_runnable, NULL, &ota_task_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_key_task_runnable */
/**
* @brief Function implementing the key_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_key_task_runnable */
void key_task_runnable(void *argument)
{
  /* USER CODE BEGIN key_task_runnable */
  key_interface.pfinit();
  /* Infinite loop */
  for(;;)
  {
    //key task
    key_interface.pftask();
  }
  /* USER CODE END key_task_runnable */
}

/* USER CODE BEGIN Header_led_task_runnable */
/**
* @brief Function implementing the led_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_led_task_runnable */
void led_task_runnable(void *argument)
{
  /* USER CODE BEGIN led_task_runnable */
  led_interface.pfinit();
  /* Infinite loop */
  for(;;)
  {
    //led task
    led_interface.pftask();
  }
  /* USER CODE END led_task_runnable */
}

/* LedCallback function */
void LedCallback(void *argument)
{
  /* USER CODE BEGIN LedCallback */
  led_interface.pftimercallback(LED_BLUE);
  /* USER CODE END LedCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
uint16_t receive_data_len;
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UNUSED(huart);
		UNUSED(Size);

  if (huart->Instance == USART1)
  {
//    uint8_t flag;
		
		//TODO:此处huart的空闲寄存器无法获取，后续查找原因
    //if(1 == __HAL_UART_GET_FLAG(huart,UART_FLAG_IDLE))
    if(HAL_UART_RXEVENT_IDLE == huart->RxEventType)
    {
      receive_data_len = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
      // DMA 传输完成后的操作
      HAL_UART_DMAStop(huart);
      // 释放信号量
      xSemaphoreGiveFromISR(binarySemaphore, &xHigherPriorityTaskWoken);
    }
  }
  // 如果在中断中唤醒了更高优先级的任务，进行任务切换
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/* USER CODE END Application */

