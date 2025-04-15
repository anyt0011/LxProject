/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <elog.h>

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "semphr.h"
#include "usart.h"

#include "bsp_rtc.h"
#include "SEGGER_RTT.h"

#define IS_IRQ()                  (__get_IPSR() != 0U)

volatile uint8_t RTT_BufferUp0[1024]={0,};

static SemaphoreHandle_t s_log_mutex = NULL;

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    TaskHandle_t async_thread = NULL;
    static SemaphoreHandle_t s_log_sync = NULL;
    static void async_output(void *arg); /* async output thread code */
#endif


/**
 * @brief Initialization EsayLogger 
 * 
 * @return ElogErrCode
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;
    SEGGER_RTT_ConfigUpBuffer(0,"Buffer0Up",(uint8_t *)&RTT_BufferUp0[0],sizeof(RTT_BufferUp0),SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    /* add your code here */
#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    const uint16_t statck_depth_words = 1024 / sizeof(StackType_t);
    const UBaseType_t priority = osPriorityLow;//tskIDLE_PRIORITY + 1; osPriorityHigh
    s_log_sync  = xSemaphoreCreateBinary();

    if(NULL == s_log_sync )
    {
        vSemaphoreDelete(s_log_sync);
        result = ELOG_CREATE_SEM_FAIL;
        return result;
    }

    if( pdPASS != xTaskCreate(
                            async_output,
                            "elog_async",
                            statck_depth_words,
                            NULL,
                            priority,
                            &async_thread))
    {
        vSemaphoreDelete(s_log_sync);
        result = ELOG_CREATE_TASK_FAIL;
        return result;
    }
#endif

    s_log_mutex = xSemaphoreCreateMutex();
    if(NULL == s_log_mutex)
    {
        vSemaphoreDelete(s_log_mutex);
        result = ELOG_CREATE_SEM_FAIL;
        return result;
    }
    
    return result;
}


/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {

    /* add your code here */

}

extern void bsp_fdb_easylog_save(const char *log,size_t size);
/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    
    /* add your code here */
    SEGGER_RTT_Write(0, log, size);
//		while(size--){
//			ITM_SendChar(*log++);
//		}
}


/**
 * output lock
 */
void elog_port_output_lock(void) {
#if (INCLUDE_xTaskGetSchedulerState  == 1 )
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        return;
    }
#endif
    if (xPortIsInsideInterrupt() || NULL==s_log_mutex) {
        return;
    }
    xSemaphoreTake(s_log_mutex, portMAX_DELAY);
}


/**
 * output unlock
 */
void elog_port_output_unlock(void) {
#if (INCLUDE_xTaskGetSchedulerState  == 1 )
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        return;
    }
#endif
    if (xPortIsInsideInterrupt() || NULL==s_log_mutex){
        return;
    }
    xSemaphoreGive(s_log_mutex);
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    
    /* add your code here */ 
   static char stamp[12] = {0};
   memset(stamp, 0, sizeof(stamp));
   #if (INCLUDE_xTaskGetSchedulerState  == 1 )
       if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
       {
   #endif
           snprintf(stamp, 12, "%d", xTaskGetTickCount());
   #if (INCLUDE_xTaskGetSchedulerState  == 1 )
       }
   #endif
    // bsp_rtc_GetSystemTime_Fmt(stamp);
    
    return stamp;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
    return "";
}


/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
    #if (INCLUDE_xTaskGetSchedulerState == 1)
        if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
            !xPortIsInsideInterrupt())
        {
    #endif
            return pcTaskGetName(xTaskGetCurrentTaskHandle());
    #if (INCLUDE_xTaskGetSchedulerState == 1)
        }
    #endif
    return "";    
}


#ifdef ELOG_ASYNC_OUTPUT_ENABLE

void elog_async_output_notice(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ( xPortIsInsideInterrupt())
    {
        xSemaphoreGiveFromISR(s_log_sync, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return;
    }

    xSemaphoreGive(s_log_sync);
}    


static void async_output(void *arg)
{
    size_t get_log_size = 0;
#ifdef ELOG_ASYNC_LINE_OUTPUT
    static char poll_get_buf[ELOG_LINE_BUF_SIZE - 4];
#else
    static char poll_get_buf[ELOG_ASYNC_OUTPUT_BUF_SIZE - 4];
#endif

    for(;;)
    {
        // wait for semaphore
        xSemaphoreTake(s_log_sync, portMAX_DELAY);

        /* polling gets and outputs the log */
        for(;;)
        {
#ifdef ELOG_ASYNC_LINE_OUTPUT
            get_log_size = elog_async_get_line_log(poll_get_buf, sizeof(poll_get_buf));
#else
            get_log_size = elog_async_get_log(poll_get_buf, sizeof(poll_get_buf));
#endif        
            if(get_log_size)
            {
                elog_port_output(poll_get_buf, get_log_size);
            }
            else
            {
                break; 
            } 
        }
    }
}
#endif


