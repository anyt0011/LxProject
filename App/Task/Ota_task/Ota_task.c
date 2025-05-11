/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file Ota_task.c
 * 
 * @par dependencies 
 * - Ota_task.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Functions related to reading and writing in the chip's flash area.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "Ota_task.h"
/* Private typedef -----------------------------------------------------------*/
osThreadId_t w25qload_taskHandle;
const osThreadAttr_t w25qload_task_attributes = {
  .name = "w25qload_task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
SemaphoreHandle_t w25qstateSemaphore;
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
QueueHandle_t newappdataQueue;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void w25qload_task_runnable(void *argument);
/* extern variables ---------------------------------------------------------*/
uint8_t *buffer1;
uint8_t *buffer2;
int32_t s32_W25qDataSize = 0;
void ota_task_runnable(void *argument)
{
  /* USER CODE BEGIN key_task_runnable */
  int32_t app_data_length;
  uint8_t rec_cmd[4];
  enum OtaState En_OtaState = OTA_REQUEST_LOAD;
  uint8_t t_u8_EepromState = 0;

  /* Infinite loop */
  for(;;)
  {
    switch(En_OtaState)
    {
        case OTA_REQUEST_LOAD:
          /*等待下载请求协议
          接收0x11， 0x22， 0x33表示进入下载
          等待时间   无限等待，
          */
          HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rec_cmd, 4);
          xSemaphoreTake(binarySemaphore, portMAX_DELAY);

          //验证数据
          if((rec_cmd[0] == 0x11) && (rec_cmd[1] == 0x22) && (rec_cmd[2] == 0x33))
          {
            //生成缓冲区
            buffer1 = (uint8_t *)pvPortMalloc(BUFFER_SIZE);
            buffer2 = (uint8_t *)pvPortMalloc(BUFFER_SIZE);
            if((NULL == buffer1) || (NULL == buffer2)){
              printf("error: malloc buffer");
              osDelay(portMAX_DELAY);
            }
            //进入数据传输阶段
            En_OtaState = OTA_LOADING;
            //在Eeprom对应位置写0x11
            t_u8_EepromState = 0x11;
            ee_WriteBytes(&t_u8_EepromState,0,1);
            newappdataQueue = xQueueCreate(2, sizeof(uint8_t *));
            w25qload_taskHandle = osThreadNew(w25qload_task_runnable, NULL, &w25qload_task_attributes);
            w25qstateSemaphore = xSemaphoreCreateMutex();
          }
          else
          {
            //释放缓冲区
            vPortFree(buffer1);
            vPortFree(buffer2);
            //下载任务，释放
            vTaskDelete(w25qload_taskHandle);
            //队列删除，互斥锁删除
            vQueueDelete(newappdataQueue);
            vSemaphoreDelete(w25qstateSemaphore);
            En_OtaState = OTA_REQUEST_LOAD;
            printf("Request download faild");
          }
        break;

        case OTA_LOADING:
            //接收数据
            app_data_length = Ymodem_Receive(buffer1,buffer2);
            if(app_data_length > 0)
            {
              W25Q64_WriteData_End();
              //释放缓冲区
              vPortFree(buffer1);
              vPortFree(buffer2);
              //进入下一状态
              En_OtaState = OTA_REQUEST_UPDATA;
              t_u8_EepromState = 0x22;
              ee_WriteBytes(&t_u8_EepromState,0,1);

              ee_WriteBytes((uint8_t *)&app_data_length,8,4);
              printf("Please respond within 30S!");
            }
            else
            {
              //释放缓冲区
              vPortFree(buffer1);
              vPortFree(buffer2);
              En_OtaState = OTA_REQUEST_LOAD;
              printf("Download Process faild,Please try again later!");
            }
        break;

        case OTA_REQUEST_UPDATA:
          /*等待下载请求协议
          接收0x44， 0x55， 0x66表示立即更新App
          等待时间   30S
          */
          HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rec_cmd, 4);
          if(xSemaphoreTake(binarySemaphore, 30000) == 1)
          {
            //验证数据
            if((rec_cmd[0] == 0x44) && (rec_cmd[1] == 0x55) && (rec_cmd[2] == 0x66))
            {
              //执行软复位
              SoftReset();
            }
            else
            {
              En_OtaState = OTA_WAIT_CPU_RESET;
              printf("Update after power back on next time.");
            }
          }
          else
          {
              En_OtaState = OTA_WAIT_CPU_RESET;
              printf("Update after power back on next time.");
          }
        break;
        
        case OTA_WAIT_CPU_RESET:
          //等待硬件复位
          osDelay(portMAX_DELAY);
        break;

        default:
        // No action
        break;
    }
  }
  /* USER CODE END key_task_runnable */
}

void w25qload_task_runnable(void *argument)
{
  //等待一个响应信号
  uint8_t * pu8_data = NULL;
  //第一帧数据后需用作初始化擦除flash的size
  xQueueReceive(newappdataQueue,&pu8_data,portMAX_DELAY); 
  xSemaphoreGive(w25qstateSemaphore);
  while(1)
  {
    xQueueReceive(newappdataQueue,&pu8_data,portMAX_DELAY);
    xSemaphoreTake(w25qstateSemaphore,0);
    if(pu8_data == NULL)
    {
        continue;
    }
    W25Q64_WriteData(pu8_data,s32_W25qDataSize);
    xSemaphoreGive(w25qstateSemaphore);
  }
}
