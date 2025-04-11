/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file bsp_led_handler.c
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
#include "bsp_led_handler.h"
#include "bsp_led_driver.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//
typedef struct
{
    uint32_t     cycle_time;
    uint32_t     blink_times;
    proportion_t proportion_on_off;
    led_index_t  led_index;
} led_event_t;

/**
 * @brief LED blink handler only call by ____event_process
 *
 * Steps:
 *
 * @param[in] self
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
static led_handler_status_t __led_blink_handler(bsp_led_handler_t *self,
                                                led_index_t        led_index)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    // 1. Check if the target has been instantiated
    // 2.Analyze the features
    {
        uint32_t     cycle_time_local;
        uint32_t     blink_times_local;
        proportion_t proportion_local;
        uint32_t     led_toggled_time;

        cycle_time_local =
            self->instances.led_instance_group[led_index]->cycle_time_ms;
        blink_times_local =
            self->instances.led_instance_group[led_index]->blink_times;
        proportion_local =
            self->instances.led_instance_group[led_index]->proportion_on_off;

        if(PROPORTION_1_1 == proportion_local)
        {
            led_toggled_time = cycle_time_local / 2;
        }
        else if(PROPORTION_1_2 == proportion_local)
        {
            led_toggled_time = cycle_time_local / 3;
        }
        else if(PROPORTION_1_3 == proportion_local)
        {
            led_toggled_time = cycle_time_local / 4;
        }
        else
        {
#ifdef DEBUG
            DEBUG_OUT("LED_ERRORPARAMETER in file:%s, line:%d\n",
                      __FILE__,
                      __LINE__);
#endif // DEBUG
            return LED_HANDLER_ERR_PARAM;
        }

        // 3. Do the operation
        for(uint32_t i = 0; i < blink_times_local; i++)
        {
            for(uint32_t j = 0; j < cycle_time_local; j++)
            {
                if(j < led_toggled_time)
                {
                    self->instances.led_instance_group[led_index]
                        ->p_led_opes_inst->pf_led_on();
                }
                else
                {
                    self->instances.led_instance_group[led_index]
                        ->p_led_opes_inst->pf_led_off();
                }
                /* delay */
                self->p_os_time_delay->pf_os_delay_ms(1);
            }
        }
    }
    return ret;
}

/**
 * @brief LED queue event process
 *
 * Steps:
 *
 * @param[in] self        : Pointer to the target of handler.
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
static led_handler_status_t __event_process(bsp_led_handler_t *self,
                                            led_event_t        msg)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    // 1. Check the target status
    // 2. Check the input parameters
    // 3. Process the receive event
    if(LED_MAX_INSTANCE_NUM < msg.led_index)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_RESERVED in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_SOURCE;
    }

    if(INIT_PATTERN == self->instances.led_instance_group[msg.led_index])
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_SOURCE in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_SOURCE;
    }

    self->instances.led_instance_group[msg.led_index]->cycle_time_ms =
        msg.cycle_time;
    self->instances.led_instance_group[msg.led_index]->blink_times =
        msg.blink_times;
    self->instances.led_instance_group[msg.led_index]->proportion_on_off =
        msg.proportion_on_off;

    ret = __led_blink_handler(self, msg.led_index);
    if(LED_HANDLER_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERROR in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERROR;
    }
    return ret;
}

/**
 * @brief Initialization LED array.
 *
 * Steps:
 *
 * @param[in]
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
static led_handler_status_t __array_init(bsp_led_driver_t *array[],
                                         uint32_t          array_size)
{
    for(uint32_t i = 0; i < array_size; i++)
    {
        array[i] = (bsp_led_driver_t *) INIT_PATTERN;
    }
    return LED_HANDLER_OK;
}

/**
 * @brief LED hander thread
 *
 * Steps:
 *
 * @param[in]
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_handler_status_t handler_thread(void *arg)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    bsp_led_handler_t   *p_led_handler;
    led_event_t          msg;
    // 1. Check the target status
    // 2. Check the input parameters
    if(NULL == arg)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_PARAM in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_PARAM;
    }
    p_led_handler = arg;
    // 3. Process event of LED queue
    for(;;)
    {
        ret = p_led_handler->p_os_queue->pf_os_queue_get(
            p_led_handler->queue_handler,
            &msg,
            0);
        if(LED_HANDLER_OK == ret)
        {
            __event_process(p_led_handler, msg);
        }
        p_led_handler->p_os_time_delay->pf_os_delay_ms(100);
    }
}

/**
 * @brief LED hander control
 *
 * Steps:
 *
 * @param[in]
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_handler_status_t handler_led_control(bsp_led_handler_t *const self,
                                         uint32_t                 cycle_time,
                                         uint32_t                 blink_times,
                                         proportion_t      proportion_on_off,
                                         led_index_t const index)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    // 1. Check the input parameters and target status
    if((NULL == self) || (cycle_time > 10000) || (blink_times > 1000) ||
       (PROPORTION_MAX <= proportion_on_off) || LED_MAX_INSTANCE_NUM < index)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_PARAM in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_PARAM;
    }
    if(false == self->is_inited)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_SOURCE in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_SOURCE;
    }

    // 2.Send event to LED queue
    led_event_t led_event = {.cycle_time        = cycle_time,
                             .blink_times       = blink_times,
                             .proportion_on_off = proportion_on_off};
    ret = self->p_os_queue->pf_os_queue_put(self->queue_handler, &led_event, 0);
    if(LED_HANDLER_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_NOMEMORY in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_NOMEMORY;
    }
    return ret;
}

/**
 * @brief Register the target of led_driver_instance.
 *
 * Steps:
 *
 * @param[in]
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_handler_status_t handler_led_register(bsp_led_handler_t *const self,
                                          bsp_led_driver_t *const  led_driver,
                                          led_index_t *const       index)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    // 1. Check the input parameters and target status
    if(NULL == self || NULL == led_driver || NULL == index)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_PARAM in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_PARAM;
    }
    if(false == self->is_inited)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_SOURCE in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_SOURCE;
    }

    // 2.Add the instance in target array
    self->p_os_critical->pf_os_critical_enter();
    if(LED_MAX_INSTANCE_NUM <= self->instances.led_instance_num)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_NOMEMORY in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        ret = LED_HANDLER_ERR_NOMEMORY;
    }
    else
    {
        self->instances.led_instance_group[self->instances.led_instance_num] =
            led_driver;
        *index = (led_index_t) self->instances.led_instance_num;
        self->instances.led_instance_num++;
    }
    self->p_os_critical->pf_os_critical_exit();

    return ret;
}

/**
 * @brief Instantiate the target of bsp_led_driver_t.
 *
 * Steps:
 *
 * @param[in]
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_handler_status_t led_handler_inst(bsp_led_handler_t *const      self,
                                      handler_os_delay_t *const     os_delay,
                                      handler_os_queue_t *const     os_queue,
                                      handler_os_critical_t *const  os_critical,
                                      handler_os_thread_t *const    os_thread,
                                      handler_time_base_ms_t *const time_base)
{
    led_handler_status_t ret = LED_HANDLER_OK;
    // 1. Check the input parameters and target status
    if(NULL == self || NULL == os_delay || NULL == os_queue ||
       NULL == os_critical || NULL == os_thread || NULL == time_base)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_PARAM in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_PARAM;
    }
    if(true == self->is_inited)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERR_SOURCE in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERR_SOURCE;
    }

    // 2. Add the interfaces
    self->p_time_base_ms  = time_base;
    self->p_os_time_delay = os_delay;
    self->p_os_queue      = os_queue;
    self->p_os_critical   = os_critical;
    self->p_os_thread     = os_thread;

    self->pf_led_controler = handler_led_control;
    self->pf_led_register  = handler_led_register;

    // 3.Initialization the target
    ret = self->p_os_queue->pf_os_queue_create(10,
                                               sizeof(led_event_t),
                                               &(self->queue_handler));
    if(LED_HANDLER_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERROR in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        return LED_HANDLER_ERROR;
    }

    self->instances.led_instance_num = 0;
    ret =
        __array_init(self->instances.led_instance_group, LED_MAX_INSTANCE_NUM);
    if(LED_HANDLER_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERROR in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        self->p_os_queue->pf_os_queue_delete(self->queue_handler);
        return LED_HANDLER_ERROR;
    }

    ret = self->p_os_thread->pf_os_thread_create(handler_thread,
                                                 "handler_thread",
                                                 128 * 4,
                                                 self,
                                                 16,
                                                 self->thread_handler);
    if(LED_HANDLER_OK != ret)
    {
#ifdef DEBUG
        DEBUG_OUT("LED_HANDLER_ERROR in file:%s, line:%d\n",
                  __FILE__,
                  __LINE__);
#endif // DEBUG
        self->p_os_queue->pf_os_queue_delete(self->queue_handler);
        self->p_os_thread->pf_os_thread_delete(self->thread_handler);
        return LED_HANDLER_ERROR;
    }

    self->is_inited = true;
    return ret;
}
//******************************** Defines **********************************//
