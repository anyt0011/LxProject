/**
* @file         stm32f411_task_key.h
* @brief		key task header file
* @details	    This file provides the interface of key task ,key type and key result.
* @author		lin jie
* @date		    2024/6/23
* @version	    v1.0
* @par Copyright(c): 	lin jie
*/

#ifndef __STM32F411_TASK_KEY_H
#define __STM32F411_TASK_KEY_H

#include "stm32f4xx.h"

typedef enum
{
    KEY_NULL,
    KEY_PRESS,
    KEY_RELEASE,
} key_state_t;


typedef enum{
    KEY_USER_KEY,
    KEY_TYPE_MAX,
}key_type_t;

typedef enum{
    KEY_NO_PRESS,
    KEY_SHORT_PRESS,
    KEY_LONG_PRESS,
}key_result_t;



typedef struct 
{
    key_type_t key_type;
    uint32_t triggle_tick;
    key_state_t state;
}key_event_t;

struct KeyInterface{
    void (*pfinit)(void);
    void (*pftask)(void);
    key_result_t (*pfget_result)(void);
};

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

extern struct KeyInterface key_interface;


#endif // __STM32F411_TASK_KEY_H
