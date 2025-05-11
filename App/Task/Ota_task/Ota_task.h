/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file Ota_task.h
 * 
 * @par dependencies 
 * - Ota_task.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Execute the Ota upgrade process
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OTA_TASK_H
#define __OTA_TASK_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Exported types ------------------------------------------------------------*/
enum OtaState
{
    OTA_REQUEST_LOAD = 0,
    OTA_LOADING,
    OTA_REQUEST_UPDATA,
    OTA_WAIT_CPU_RESET
};
/* Exported variable --------------------------------------------------------*/
extern QueueHandle_t newappdataQueue;
extern SemaphoreHandle_t w25qstateSemaphore;
extern int32_t s32_W25qDataSize;
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define BUFFER_SIZE                     1030
/* Exported functions ------------------------------------------------------- */
extern void ota_task_runnable(void *argument);
#endif
