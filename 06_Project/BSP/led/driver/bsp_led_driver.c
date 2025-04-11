/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file bsp_led_driver.c
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

//******************************* Includes **********************************//
#include "bsp_led_driver.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//

/**
 * @brief Initialization the bsp_led_driver_t target.
 *
 * Steps:
 *  1. Analyze the target parameters
 *
 * @param[in] self : Pointer to the target of handler.
 *
 * @return led_status_t : Status of the function.
 *
 * */
static led_status_t led_driver_init(bsp_led_driver_t *const self)
{
    led_status_t ret = LED_OK;

    if(NULL == self)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_ERRORPARAMETER in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_ERROR_PARAMETER;
    }
    self->p_led_opes_inst->pf_led_off();
    return ret;
}

/**
 * @brief Instantiation the bsp_led_driver_t target
 *
 * Steps:
 *
 * @param[in]
 *
 * @return
 *
 * */
led_status_t led_driver_inst(bsp_led_driver_t *const self,
                             led_operations_t *const led_ops)
{
    led_status_t ret = LED_OK;
    // 1.Check the input parameters
    if(NULL == self || NULL == led_ops)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_ERRORPARAMETER in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_ERROR_PARAMETER;
    }
    // 2. Check if target has been instantiated
    if(true == self->is_inited)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_ERROR_RESOURCES in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_ERROR_RESOURCES;
    }
    // 3. Add the interface
    self->p_led_opes_inst = led_ops;

    // 4. Initialization the target
    self->blink_times       = 0;
    self->cycle_time_ms     = 0;
    self->proportion_on_off = PROPORTION_1_1;
    ret                     = led_driver_init(self);
    if(LED_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED init failed:%s, line:%d\n", __FILE__, __LINE__);
#endif // DEBUG
        self->p_led_opes_inst = NULL;
        return ret;
    }
    self->is_inited = true;
    return ret;
}

//******************************** Defines **********************************//
