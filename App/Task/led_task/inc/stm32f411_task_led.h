/**
* @file         stm32f411_task_led.h
* @brief		led task header file
* @details	    This file provides the interface of led task and led type.
* @author		lin jie
* @date		    2024/6/23
* @version	    v1.0
* @par Copyright(c): 	lin jie
*/

#ifndef __STM32F411_TASK_LED_H
#define __STM32F411_TASK_LED_H

#include "stm32f4xx.h"

typedef enum{
    TIME_STOP,
    TIME_RUNNING,
}led_tim_state_t;

typedef enum{
    LED_BLUE,
    LED_TYPE_MAX,
}led_type_t;

struct LedInterface{
    void (*pfinit)(void);
    void (*pftask)(void);
    void (*pfon)(led_type_t led);
    void (*pfoff)(led_type_t led);
    void (*pftoggle)(led_type_t led);
    void (*pftimercallback)(led_type_t led);
};

extern struct LedInterface led_interface;

#endif /* __STM32F411_TASK_LED_H */
