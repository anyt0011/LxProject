/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file bsp_led_hander.h
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
#ifndef __BSP_LED_HANDLER_H__
#define __BSP_LED_HANDLER_H__

//******************************* Includes **********************************//
#include "bsp_led_driver.h"

#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//
#define OS_SUPPORTING /* OS_SUPPORTING depending on OS availabel */
#define INIT_PATTERN  (bsp_led_driver_t *) (0xA6A6A6A6)
/* Init space with this pattern */

typedef struct bsp_led_handler bsp_led_handler_t;

typedef enum
{
    HANDLER_NOT_INITED = 1, /* LED handler is not inited */
    HANDLER_INITED     = 0, /* LED handler is inited     */
} led_handler_init_t;

typedef enum
{
    LED_1 = 0, /* LED index number which passed to app */
    LED_2,
    LED_3,
    LED_4,
    LED_5,
    LED_6,
    LED_7,
    LED_8,
    LED_9,
    LED_10,
    LED_MAX_INSTANCE_NUM,
} led_index_t;

/**
 * @brief LED处理状态枚举类型定义
 *
 * 此枚举类型用于定义LED处理过程中可能遇到的各种状态，包括成功、通用错误以及特定错误。
 * 它为LED处理函数提供了一种标准化的方式来指示操作的结果状态。
 */
typedef enum
{
    LED_HANDLER_OK = 0,       /**< 成功: LED处理操作按预期完成 */
    LED_HANDLER_ERROR,        /**< 通用错误: 表示LED处理中遇到的非特定错误 */
    LED_HANDLER_ERR_TIMEOUT,  /**< 超时错误: 处理操作未能在预期时间内完成 */
    LED_HANDLER_ERR_SOURCE,   /**< 来源错误: 错误源自不正确的源或调用上下文 */
    LED_HANDLER_ERR_PARAM,    /**< 参数错误: 提供给LED处理函数的参数错误 */
    LED_HANDLER_ERR_NOMEMORY, /**< 内存错误: 操作因内存不足而失败 */
    LED_HANDLER_ERR_ISR,      /**< 中断服务例程错误: 在中断服务例程中操作失败 */
    LED_HANDLER_ERR_RESERVED, /**< 预留错误: 用作未来扩展的占位符 */
    LED_HANDLER_STATUS_MAX
} led_handler_status_t;

typedef struct
{
    led_handler_status_t (*pf_get_time_ms)(uint32_t *const);
} handler_time_base_ms_t;

typedef struct
{
    led_handler_status_t (*pf_os_delay_ms)(const uint32_t);
} handler_os_delay_t;

typedef struct
{
    /* OS queue create */
    led_handler_status_t (*pf_os_queue_create)(uint32_t const item_num,
                                               uint32_t const item_size,
                                               void **const   queue_handler);
    /* OS queue put */
    led_handler_status_t (*pf_os_queue_put)(void *const queue_handler,
                                            void *const item,
                                            uint32_t    timeout);

    /* OS queue get */
    led_handler_status_t (*pf_os_queue_get)(void *const queue_handler,
                                            void *const msg,
                                            uint32_t    timeout);

    /* OS queue delete */
    led_handler_status_t (*pf_os_queue_delete)(void *const queue_handler);
} handler_os_queue_t;

typedef struct
{
    led_handler_status_t (*pf_os_critical_enter)(void); /*  */
    led_handler_status_t (*pf_os_critical_exit)(void);  /*  */
} handler_os_critical_t;

typedef struct
{
    led_handler_status_t (*pf_os_thread_create)(void *const       task_core,
                                                const char *const task_name,
                                                const uint32_t    stack_depth,
                                                void *const       parameters,
                                                uint32_t          priority,
                                                void *const thread_handler);
    led_handler_status_t (*pf_os_thread_delete)(void *const);
} handler_os_thread_t;

typedef struct
{
    uint32_t          led_instance_num;
    bsp_led_driver_t *led_instance_group[LED_MAX_INSTANCE_NUM];
} instance_registered_t;

typedef led_handler_status_t (*pf_handler_led_control_t)(
    bsp_led_handler_t *const,
    uint32_t,
    uint32_t,
    proportion_t,
    led_index_t const);

typedef led_handler_status_t (*pf_handler_led_register_t)(
    bsp_led_handler_t *const,
    bsp_led_driver_t *const,
    led_index_t *const);

struct bsp_led_handler
{
    /* Internal runtime data for internal status */
    uint8_t               is_inited;
    instance_registered_t instances;
    void                 *queue_handler;
    void                 *thread_handler;

    /* Interfaces for internal use */
    handler_time_base_ms_t *p_time_base_ms;
    handler_os_delay_t     *p_os_time_delay;
    handler_os_queue_t     *p_os_queue;
    handler_os_critical_t  *p_os_critical;
    handler_os_thread_t    *p_os_thread;

    /* Interfaces for external use */
    pf_handler_led_control_t  pf_led_controler;
    pf_handler_led_register_t pf_led_register;
};

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
/**
 * @brief Instantiates the bsp_led_handler_t target.
 *
 * Steps:
 *  1. Adds Core interfaces into bsp_led_driver instance target.
 *  2. Adds OS interfaces into bsp_led_driver instance target.
 *  3. Adds timebase interfaces into bsp_led_driver instance target.
 *
 * @param[in] self        : Pointer to the target of handler.
 * @param[in] os_delay    : Pointer to the os_delay_interface.
 * @param[in] os_queue    : Pointer to the os_queue_interface.
 * @param[in] os_critical : Pointer to the os_critical_interface.
 * @param[in] os_thread   : Pointer to the os_thread_interface.
 * @param[in] time_base   : Pointer to the time_base_interface.
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_handler_status_t led_handler_inst(bsp_led_handler_t *const      self,
                                      handler_os_delay_t *const     os_delay,
                                      handler_os_queue_t *const     os_queue,
                                      handler_os_critical_t *const  os_critical,
                                      handler_os_thread_t *const    os_thread,
                                      handler_time_base_ms_t *const time_base);
//******************************** Declaring ********************************//

#endif
