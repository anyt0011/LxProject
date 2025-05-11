/**
 * @file         stm32f411_task_key.c
 * @brief		key task
 * @details	    This file provides all the key task functions to check the user's operation for key.
 * @author		lin jie
 * @date		    2024/6/23
 * @version	    v1.0
 * @par Copyright(c): 	lin jie
 */

#include "stm32f411_task_key.h"
#include "main.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"

#define KEY_RESULT_QUEUE_SIZE 1
#define KEY_EVENT_QUEUE_SIZE 2

static void key_init(void);
static void key_task(void);
static key_result_t key_pfget_result(void);

// typedef enum
// {
//     KEY_IDLE,
//     KEY_TRIGGER,
//     KEY_SHORT_EVENT,
//     KEY_LONG_EVENT,
//     KEY_DOUBLE_EVENT,
// } key_state_t;

struct KeyClass
{
    key_state_t key_state;
    key_result_t key_result;
};

struct KeyClass keyimp[KEY_TYPE_MAX];

static QueueHandle_t g_key_event_queue;
static QueueHandle_t g_key_result_queue;

struct KeyInterface key_interface = {
    .pfinit = key_init,
    .pftask = key_task,
    .pfget_result = key_pfget_result,
};

/**
 * @brief  get key event
 *
 * @param  event key event
 *
 * @retval get event or not
 *         0   not
 *         1   get
 */
static key_result_t key_pfget_result(void)
{
    key_result_t result;
    if (pdTRUE == xQueueReceive(g_key_result_queue, &result, portMAX_DELAY))
        return result;
    else
        return KEY_NO_PRESS;
}

/**
 * @brief  key task loop
 *         use for check user's operation
 *
 * @param  void
 *
 * @retval void
 */
static void key_task(void)
{
    key_result_t result;
    key_event_t *event;
    static uint32_t key_triggle_tick;
    uint32_t key_interval_tick = 0;
    // get key event from queue
    xQueueReceive(g_key_event_queue, &event, portMAX_DELAY);

    for (key_type_t key = KEY_USER_KEY; key < KEY_TYPE_MAX; key++)
    {
        // search for compare type
        if (KEY_USER_KEY == event->key_type)
        {
            switch (event->state)
            {
            case KEY_PRESS:
                // if first press key
                key_triggle_tick = event->triggle_tick;
                break;

            case KEY_RELEASE:
                key_interval_tick = event->triggle_tick - key_triggle_tick;
                if( event->triggle_tick < key_triggle_tick)
                {
                    //Clear tick
                    //Data crosses the line
                    key_triggle_tick = 0;
                }
                else if (key_interval_tick < 2000)
                {
                    //Clear tick
                    key_triggle_tick = 0;
                    // if key is pressed after release in 200ms
                    result = KEY_SHORT_PRESS;
                    printf("short press\r\n");
                    xQueueSend(g_key_result_queue, &result, portMAX_DELAY);
                }
                else if (key_interval_tick < 5000)
                {
                    //Clear tick
                    key_triggle_tick = 0;
                    // send result to queue
                    result = KEY_LONG_PRESS;
                    printf("long press\r\n");
                    xQueueSend(g_key_result_queue, &result, portMAX_DELAY);
                }
                else
                {
                    //Clear tick
                    key_triggle_tick = 0;
                    // send result to queue
                    result = KEY_NO_PRESS;
                    printf("The pressing time is too long, and the behavior is invalid.\r\n");
                    xQueueSend(g_key_result_queue, &result, portMAX_DELAY);
                }
                break;

            default:
                // No action
                break;
            }
        }
    }
}

/**
 * @brief  key init function
 *
 * @param  key key type
 *
 * @retval key_result   user's operation for key
 */
static void key_init(void)
{
    g_key_event_queue = xQueueCreate(KEY_EVENT_QUEUE_SIZE, sizeof(uint32_t));
    g_key_result_queue = xQueueCreate(KEY_RESULT_QUEUE_SIZE, sizeof(key_result_t));
}

void GPIO_Triggle_set(key_state_t state)
{
    static key_event_t event1;
    static key_event_t *event = &event1;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // printf("key release\r\n");
    if (state != event->state)
    {
        event->state = state;
        if(KEY_RELEASE == event->state)
        {
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        }
        else if(KEY_PRESS == event->state)
        {
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        }
        else
        {
            
        }
        // write event
        event->key_type = KEY_USER_KEY;
        event->triggle_tick = HAL_GetTick();

        // set EXTI interrupt mode
        GPIO_InitStruct.Pin = User_Key_Pin;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(User_Key_GPIO_Port, &GPIO_InitStruct);

        // write event into queue
        xQueueSendFromISR(g_key_event_queue, &event, NULL);

        printf("key trigle : %d\r\n", event->state);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == User_Key_Pin)
    {
        if(HAL_GPIO_ReadPin(User_Key_GPIO_Port, User_Key_Pin))
        {
            GPIO_Triggle_set(KEY_RELEASE);
        }
        else
        {
            GPIO_Triggle_set(KEY_PRESS);
        }
        
    }
}
