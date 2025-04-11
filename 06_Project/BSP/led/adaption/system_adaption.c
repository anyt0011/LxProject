/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file system_adaption.c
 *
 * @par dependencies
 * - stdio.h
 * - stdint.h
 *
 * @author Anyt
 *
 * @brief Integrate all the resources in the system and enable them to work.
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
#include "system_adaption.h"
#include "FreeRTOS.h"
#include "bsp_led_driver.h"
#include "bsp_led_handler.h"
#include "queue.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//
// __attribute__((used, section("bsp_target"))) bsp_led_handler_t sct_led_handler;
// __attribute__((used, section("bsp_target"))) bsp_led_driver_t  sct_led_driver;

bsp_led_handler_t sct_led_handler;
bsp_led_driver_t  sct_led_driver;

static bsp_led_handler_t s_led_handler;
static bsp_led_driver_t  s_led_driver_1;
static bsp_led_driver_t  s_led_driver_2;

led_status_t led_on_myown(void)
{
    printf("led is on\r\n");
    return LED_OK;
}

led_status_t led_off_myown(void)
{
    printf("led is off\r\n");
    return LED_OK;
}

led_operations_t led_operations_myown = {.pf_led_off = led_off_myown,
                                         .pf_led_on  = led_on_myown};

led_handler_status_t os_delay_ms_handler_1(uint32_t const delay_time)
{
    vTaskDelay(delay_time);
    return LED_HANDLER_OK;
}

handler_os_delay_t handler_1_os_delay = {.pf_os_delay_ms =
                                             os_delay_ms_handler_1};

/**
 * @brief
 *
 * Steps:
 *
 * @param[in]
 *
 * @return
 *
 * */
led_handler_status_t os_queue_create_handler_1(uint32_t const item_num,
                                               uint32_t const item_size,
                                               void **const   queue_handler)
{
    xQueueHandle temp_queue_handle = NULL;
    temp_queue_handle              = xQueueCreate(item_num, item_size);
    if(NULL == temp_queue_handle)
    {
        return LED_HANDLER_ERR_SOURCE;
    }
    else
    {
        *queue_handler = temp_queue_handle;
        return LED_HANDLER_OK;
    }
}

led_handler_status_t os_queue_put_handler_1(void *const queue_handler,
                                            void *const item,
                                            uint32_t    timeout)
{
    if(NULL == queue_handler || NULL == item || timeout > portMAX_DELAY)
    {
        return LED_HANDLER_ERR_SOURCE;
    }
    else
    {
        if(xQueueSend(queue_handler, item, timeout) != pdTRUE)
        {
            return LED_HANDLER_ERROR;
        }
    }
    return LED_HANDLER_OK;
}

led_handler_status_t os_queue_get_handler_1(void *const queue_handler,
                                            void *const msg,
                                            uint32_t    timeout)
{
    if(NULL == queue_handler || NULL == msg || timeout > portMAX_DELAY)
    {
        return LED_HANDLER_ERR_PARAM;
    }

    if(xQueueReceive(queue_handler, msg, timeout) != pdTRUE)
    {
        return LED_HANDLER_ERROR;
    }

    return LED_HANDLER_OK;
}

led_handler_status_t os_queue_delete_handler_1(void *const queue_handler)
{
    if(NULL == queue_handler)
    {
        return LED_HANDLER_ERR_PARAM;
    }
    vQueueDelete(queue_handler);
    return LED_HANDLER_OK;
}

handler_os_queue_t handler_1_os_queue = {
    .pf_os_queue_create = os_queue_create_handler_1,
    .pf_os_queue_get    = os_queue_get_handler_1,
    .pf_os_queue_put    = os_queue_put_handler_1,
    .pf_os_queue_delete = os_queue_delete_handler_1};

led_handler_status_t os_critical_enter_handler_1(void)
{
    vPortEnterCritical();
    return LED_HANDLER_OK;
}

led_handler_status_t os_critical_exit_handler_1(void)
{
    vPortExitCritical();
    return LED_HANDLER_OK;
}

handler_os_critical_t handler_1_os_critical = {
    .pf_os_critical_enter = os_critical_enter_handler_1,
    .pf_os_critical_exit  = os_critical_exit_handler_1};

led_handler_status_t get_time_ms_handler_1(uint32_t *const p_os_tick)
{
    if(NULL == p_os_tick)
    {
        return LED_HANDLER_ERR_PARAM;
    }
    *p_os_tick = HAL_GetTick();
    return LED_HANDLER_OK;
}
extern handler_time_base_ms_t handler_1_time_base = {.pf_get_time_ms =
                                                         get_time_ms_handler_1};

led_handler_status_t thread_create_handler_1(void *const       task_code,
                                             const char *const task_name,
                                             const uint32_t    stack_depth,
                                             void *const       parameters,
                                             uint32_t          priority,
                                             void *const       task_handler)
{
    BaseType_t ret = pdPASS;
    ret            = xTaskCreate((TaskFunction_t) task_code,
                      task_name,
                      stack_depth,
                      parameters,
                      priority,
                      task_handler);
    if(pdPASS != ret)
    {
        return LED_HANDLER_ERROR;
    }
    return LED_HANDLER_OK;
}

led_handler_status_t thread_delete_handler_1(void *const task_handler)
{

    if(NULL == task_handler)
    {
        return LED_HANDLER_ERR_PARAM;
    }
    vTaskDelete((TaskHandle_t) task_handler);
    return LED_HANDLER_OK;
}

handler_os_thread_t handler_1_os_thread = {
    .pf_os_thread_create = thread_create_handler_1,
    .pf_os_thread_delete = thread_delete_handler_1};

void Test_1(void)
{
    // Handler
    led_handler_status_t ret  = LED_HANDLER_OK;
    ret                      |= led_handler_inst(&s_led_handler,
                            &handler_1_os_delay,
                            &handler_1_os_queue,
                            &handler_1_os_critical,
                            &handler_1_os_thread,
                            &handler_1_time_base);
    // Driver
    led_status_t ret1 = LED_OK;

    ret1 |= led_driver_inst(&s_led_driver_1, &led_operations_myown);
    ret1 |= led_driver_inst(&s_led_driver_2, &led_operations_myown);
    // Integrated Test
    led_index_t handler_1_led_index_1;
    ret |= s_led_handler.pf_led_register(&s_led_handler,
                                         &s_led_driver_1,
                                         &handler_1_led_index_1);

    led_index_t handler_1_led_index_2;
    ret |= s_led_handler.pf_led_register(&s_led_handler,
                                         &s_led_driver_2,
                                         &handler_1_led_index_2);

    // API Test
    s_led_handler.pf_led_controler(&s_led_handler,
                                   10U,
                                   2U,
                                   PROPORTION_1_2,
                                   handler_1_led_index_1);

    if(LED_HANDLER_OK != ret)
    {
        printf("Test1 failed\r\n");
    }
}

/* SCT test */
void Test_2(void)
{
    // Handler
    led_handler_status_t ret = LED_HANDLER_OK;

    // Integrated Test
    led_index_t handler_1_led_index_1;
    ret |= sct_led_handler.pf_led_register(&sct_led_handler,
                                           &sct_led_driver,
                                           &handler_1_led_index_1);

    // API Test
    sct_led_handler.pf_led_controler(&sct_led_handler,
                                     10U,
                                     2U,
                                     PROPORTION_1_1,
                                     handler_1_led_index_1);

    if(LED_HANDLER_OK != ret)
    {
        printf("Test2 failed\r\n");
    }
}

void Test_3(void)
{
    // Handler
    led_handler_status_t ret  = LED_HANDLER_OK;
    ret                      |= led_handler_inst(&sct_led_handler,
                            &handler_1_os_delay,
                            &handler_1_os_queue,
                            &handler_1_os_critical,
                            &handler_1_os_thread,
                            &handler_1_time_base);
    // Driver
    led_status_t ret1 = LED_OK;
    ret1 |= led_driver_inst(&sct_led_driver, &led_operations_myown);

    // Integrated Test
    led_index_t handler_1_led_index_1;
    ret |= sct_led_handler.pf_led_register(&sct_led_handler,
                                           &sct_led_driver,
                                           &handler_1_led_index_1);

    // API Test
    sct_led_handler.pf_led_controler(&sct_led_handler,
                                     10U,
                                     1U,
                                     PROPORTION_1_1,
                                     handler_1_led_index_1);

    if(LED_HANDLER_OK != ret || LED_OK != ret1)
    {
        printf("Test3 failed\r\n");
    }
}

/**
 * @brief Init all the resources.
 *
 * Steps:
 *  1. mix up all the resources in this system.
 *
 * @param[in] self        :
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_status_t system_init_resources(void) { return LED_OK; }

//******************************** Defines **********************************//
