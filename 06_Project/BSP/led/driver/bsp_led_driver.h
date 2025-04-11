/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file bsp_led_driver.h
 *
 * @par dependencies
 * - stdio.h
 * - stdint.h
 *
 * @author Anyt
 *
 * @brief Provide the HAL APIs of LED and corresponding opetions.
 *
 * Processing flow:
 *
 * call directly.
 *
 * @version V1.0 2025-04-05
 *
 * @note 1 tab == 4 spaces!
 * @warning
 *****************************************************************************/
#ifndef __BSP_LED_DRIVER_H__
#define __BSP_LED_DRIVER_H__

//******************************* Includes **********************************//
#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//
#define DEBUG                                    /* Enable DBG info */
#define DEBUG_OUT(X, ...) printf(X, __VA_ARGS__) /* DEBUG output*/

typedef struct bsp_led_driver bsp_led_driver_t;

typedef enum
{
    LED_OK = 0,          // 表示LED操作成功。
    LED_ERROR,           // 表示LED操作中出现了一般错误。
    LED_ERROR_TIMEOUT,   // 表示LED操作超时。
    LED_ERROR_RESOURCES, // 表示资源不足，无法完成LED操作。
    LED_ERROR_PARAMETER, // 表示传入的参数无效。
    LED_ERROR_NOMEMORY,  // 表示内存不足，无法完成LED操作。
    LED_ERROR_ISR,       // 表示ISR（中断服务例程）中发生了错误。
    LED_STATUS_MAX       // 最大枚举值
} led_status_t;

typedef enum
{
    PROPORTION_1_3 = 0, // ON/OFF ratio 1:3
    PROPORTION_1_2,     // ON/OFF ratio 1:2
    PROPORTION_1_1,     // ON/OFF ratio 1:1
    PROPORTION_MAX      // 最大枚举值
} proportion_t;

typedef struct
{
    led_status_t (*pf_led_on)(void);  /*  */
    led_status_t (*pf_led_off)(void); /*  */
} led_operations_t;

struct bsp_led_driver
{
    /* Internal state */
    uint8_t is_inited;
    /* Blink configuration */
    uint32_t     cycle_time_ms;
    uint32_t     blink_times;
    proportion_t proportion_on_off;
    /* Internal Pointers */
    led_operations_t *p_led_opes_inst;
};

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
/**
 * @brief Instantiates the bsp_led_driver_t target.
 *
 * Steps:
 *  1. Adds Core interfaces into bsp_led_driver instance target.
 *  2. Adds OS interfaces into bsp_led_driver instance target.
 *  3. Adds timebase interfaces into bsp_led_driver instance target.
 *
 * @param[in] self        : Pointer to the target of handler.
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_status_t led_driver_inst(bsp_led_driver_t *const self,
                             led_operations_t *const led_ops);

//******************************** Declaring ********************************//
#endif
