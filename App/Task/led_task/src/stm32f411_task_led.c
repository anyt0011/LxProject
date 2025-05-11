/**
 * @file         stm32f411_task_led.c
 * @brief		led task
 * @details	    This file provides all the led task functions.
 * @author		lin jie
 * @date		    2024/6/23
 * @version	    v1.0
 * @par Copyright(c): 	lin jie
 */

#include "stm32f411_task_led.h"
#include "stm32f411_task_key.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"
#define LED_TIM_QUEUE_SIZE 1

extern osTimerId_t LedTimerHandle;
static void led_init(void);
static void led_task(void);
static void led_on(led_type_t led);
static void led_off(led_type_t led);
static void led_toggle(led_type_t led);
static void led_timer_callback(led_type_t led);
struct LedClass
{
    uint8_t led_state;
};

struct LedClass led_imp[LED_TYPE_MAX];

static QueueHandle_t g_led_tim_queue;

struct LedInterface led_interface = {
    .pfinit = led_init,
    .pftask = led_task,
    .pfon = led_on,
    .pfoff = led_off,
    .pftoggle = led_toggle,
    .pftimercallback = led_timer_callback,
};

/**
 * @brief  led task loop
 *
 * @param  void
 *
 * @retval void
 */
static void led_timer_callback(led_type_t led)
{
    if(led == LED_BLUE)
    {
        led_tim_state_t led_tim_state = TIME_STOP;
        xQueueSendFromISR(g_led_tim_queue, &led_tim_state, NULL);
    }
}

/**
 * @brief  led task loop
 *
 * @param  void
 *
 * @retval void
 */
static void led_task(void)
{
    key_result_t result;
    led_tim_state_t led_tim_state;
    result = key_interface.pfget_result();

    switch (result)
    {
        case KEY_SHORT_PRESS:
            led_interface.pftoggle(LED_BLUE);
            osTimerStart(LedTimerHandle,100);
            //优化使用二值信号量
            xQueueReceive(g_led_tim_queue, &led_tim_state, portMAX_DELAY);
            led_interface.pftoggle(LED_BLUE);
        break;

        case KEY_LONG_PRESS:
            // blink 3 times
            for (char i = 0; i < 20; i++)
            {
                led_interface.pftoggle(LED_BLUE);
                osTimerStart(LedTimerHandle,100);
                //优化使用二值信号量
                xQueueReceive(g_led_tim_queue, &led_tim_state, portMAX_DELAY);
            }
            break;

        case KEY_NO_PRESS:
            // No action
        break;

        default:
            // No action
        break;
    }
}

/**
 * @brief  led toggle
 *
 * @param  led type of led
 *
 * @retval void
 */
static void led_toggle(led_type_t led)
{
    if (false == led_imp[LED_BLUE].led_state)
    {
        led_imp[LED_BLUE].led_state = true;
        HAL_GPIO_WritePin(User_Led_GPIO_Port, User_Led_Pin, GPIO_PIN_SET);
    }
    else
    {
        led_imp[LED_BLUE].led_state = false;
        HAL_GPIO_WritePin(User_Led_GPIO_Port, User_Led_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief  led off
 *
 * @param  led type of led
 *
 * @retval void
 */
static void led_off(led_type_t led)
{
    if (false == led_imp[LED_BLUE].led_state)
    {
        led_imp[LED_BLUE].led_state = true;
        HAL_GPIO_WritePin(User_Led_GPIO_Port, User_Led_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  led on
 *
 * @param  led type of led
 *
 * @retval void
 */
static void led_on(led_type_t led)
{
    if (true == led_imp[LED_BLUE].led_state)
    {
        led_imp[LED_BLUE].led_state = false;
        HAL_GPIO_WritePin(User_Led_GPIO_Port, User_Led_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief  led init function
 *
 * @param  void
 *
 * @retval void
 */
static void led_init(void)
{
    g_led_tim_queue = xQueueCreate(LED_TIM_QUEUE_SIZE, sizeof(led_tim_state_t));
    led_off(LED_BLUE);
}
