/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file drv_adapter_port_temphumioment.c
 * 
 * @par dependencies 
 * - drv_adapter_port_temphumioment.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 * 
 * Processing flow:
 * 
 * 1.The aht21_inst function will instantiate the bsp_aht21_driver_t object and
 * with the needed funtion interface. 
 * 
 * 2.Then the users could all the IOs from instances of bsp_aht21_driver_t.
 * 
 * @version V1.0 2023-12-03
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
//******************************** Includes *********************************//
#include "main.h"
#include "cmsis_os.h"
#include "event_groups.h"
#include "osal.h"
#include "drv_adapter_port_temp_humi.h"
#include "ec_bsp_temp_humi_xxx_handler.h"
#include "iic_hal.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define AHT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()
//******************************** Defines **********************************//

//******************************** Variables ********************************//
static float s_temperature = 0;
static float s_humi = 0;

iic_bus_t  iic_bus_instance = {
    .IIC_SDA_PORT = GPIOB,
    .IIC_SDA_PIN = GPIO_PIN_13,
    .IIC_SCL_PORT = GPIOB,
    .IIC_SCL_PIN = GPIO_PIN_14
  };

/* Definitions for tempHandlerTask */
osThreadId_t tempHandlerTaskHandle;
const osThreadAttr_t tempHandlerTask_attributes = {
  .name = "tempHandlerTask",
  .stack_size = 128 * 6,
  .priority = (osPriority_t) osPriorityNormal,
};

/*事件组句柄*/
EventGroupHandle_t xtemphumi_event_flags_handle;

/*定义事件位*/
#define EVENT_TEMP      (1 << 0)
#define EVENT_HUMI      (1 << 1)
#define EVENT_TEMPHUMI  (1 << 2)

//******************************** Variables ********************************//

//******************************** Functions ********************************//
/***********************************************************************************
 *                 Static Declarations For temphumionment
 ***********************************************************************************/
static void temphumi_drv_init(temphumi_drv_t *dev);
static void temphumi_drv_deinit(temphumi_drv_t *dev);
static void temphumi_drv_read_temp(temphumi_drv_t * dev, float *temp);
static void temphumi_drv_read_humi(temphumi_drv_t * dev, float *humi);
static void temphumi_drv_read_temp_and_humi(temphumi_drv_t * dev, float *temp, float *humi);
static void temp_humi_callback(float *temperature, float *humidity);
/***********************************************************************************
 *                 Public Implements
 ***********************************************************************************/
void drv_adapter_temphumi_register(void)
{
    temphumi_drv_t _temphumi_drv = {
        .idx = 0,
        .dev_id = 0,
        .temphumi_drv_init = temphumi_drv_init,
        .temphumi_drv_deinit = temphumi_drv_deinit,
        .temphumi_drv_read_temp = temphumi_drv_read_temp,
        .temphumi_drv_read_humi = temphumi_drv_read_humi,
        .temphumi_drv_read_temp_and_humi = temphumi_drv_read_temp_and_humi,
    };

    drv_adapter_temphumi_reg(0, &_temphumi_drv);
}

/***********************************************************************************
 *                 Static Implements for Display
 ***********************************************************************************/
/* USER CODE BEGIN FunctionPrototypes */
aht21_status_t iic_init_myown(void * bus)
{
  AHT_CLK_ENABLE;
  IICInit(&iic_bus_instance);
  //Could check the register of IIC if required
  return AHT21_OK;
}

aht21_status_t iic_start_myown(void * bus)
{
        IICStart(&iic_bus_instance);
        return AHT21_OK;
}

aht21_status_t iic_stop_myown(void * bus)
{
        IICStop(&iic_bus_instance);
        return AHT21_OK;
}

aht21_status_t pfiic_wait_ack_myown(void * bus)
{
        unsigned char ret = SUCCESS; // should be ErrorStatus but IICWaitAck(X)
        ret = IICWaitAck(&iic_bus_instance);
        if(SUCCESS == ret)
        {
            return AHT21_OK;
        } else {
            return AHT21_ERRORTIMEOUT;
        }
}


aht21_status_t iic_send_ack_myown(void * bus)
{
        IICSendAck(&iic_bus_instance);
        return AHT21_OK;
}

aht21_status_t iic_send_no_ack_myown(void * bus)
{
        IICSendNotAck(&iic_bus_instance);
        return AHT21_OK;
}

aht21_status_t iic_send_byte_myown(void * bus, uint8_t data)
{
        IICSendByte(&iic_bus_instance, data);
        return AHT21_OK;
}

aht21_status_t iic_receive_byte_myown(void * bus, uint8_t * const data)
{
        *data = IICReceiveByte(&iic_bus_instance);
    return AHT21_OK;
}
static void temphumi_drv_init(temphumi_drv_t *dev)
{
    // 构造iic总线实例
    static iic_driver_interface_t iic_driver_interface = {
        .pf_iic_init         = iic_init_myown,
        .pf_iic_start        = iic_start_myown,
        .pf_iic_stop         = iic_stop_myown,
        .pf_iic_wait_ack     = pfiic_wait_ack_myown,
        .pf_iic_send_no_ack  = iic_send_no_ack_myown,
        .pf_iic_send_byte    = iic_send_byte_myown,
        .pf_iic_receive_byte = iic_receive_byte_myown,
        .pf_iic_send_ack     = iic_send_ack_myown,
        .pf_critical_enter   = (aht21_status_t (*)(void))vPortEnterCritical,
        .pf_critical_exit    = (aht21_status_t (*)(void))vPortExitCritical
    };
        
    static yield_interface_t yield_interface =
    {
        .pf_rtos_yield       = osal_task_delay_ms
    };

    // 时间基准
    static timebase_interface_t timebase_interface =
    {
        .pf_get_tick_count   = HAL_GetTick
    };
    // RTOS提供的接口
    static temp_humi_handler_os_interface_t os_interface =  
    {
        .os_delay_ms         = osal_task_delay_ms,
        .os_queue_create     = osal_queue_create,
        .os_queue_put        = osal_queue_send,
        .os_queue_get        = osal_queue_receive
    };
        

    static temp_humi_handler_all_input_arg_t input_arg = 
    {
        .iic_driver_interface = &iic_driver_interface,
        .timebase_interface   = &timebase_interface,
        .os_interface         = &os_interface,
        .yield_interface      = &yield_interface
    };
    //创建aht21handler线程
    tempHandlerTaskHandle = osThreadNew(temp_humi_handler_thread,\
                                              (void *)&input_arg,\
                                      &tempHandlerTask_attributes);
    //创建事件唤醒
    xtemphumi_event_flags_handle = xEventGroupCreate();
    if (xtemphumi_event_flags_handle == NULL) 
    {
        // 事件组创建失败
        while(1);
    }
}

static void temphumi_drv_deinit(temphumi_drv_t *dev)
{
    //销毁线程
}
static void temphumi_drv_read_temp(temphumi_drv_t * dev, float *temp)
{
    EventBits_t uxBits;
    const EventBits_t uxBitsToWaitFor = EVENT_TEMP;
    (void)dev;

    //读取温度
    temp_humi_xxx_event_t event = {
        // .temperature = &temperature,
        // .humidity = &humidity,
        .lifetime = 10000,
        // .timestamp = input_arg->timebase_interface->pfget_tick_count,
        .type = TEMP_HUMI_EVENT_TEMP,
        .pfcallback = temp_humi_callback
    };
    bsp_temp_humi_xxx_read(&event);
    //事件唤醒
    uxBits = xEventGroupWaitBits(
            xtemphumi_event_flags_handle,        // 事件组句柄
            uxBitsToWaitFor,    // 要等待的事件位
            pdTRUE,             // 退出时清除事件位
            pdTRUE,             // 所有事件位都要设置
            portMAX_DELAY       // 无限期等待
        );
    if ((uxBits & uxBitsToWaitFor) == uxBitsToWaitFor) 
    {
        *temp = s_temperature;
    }
}
static void temphumi_drv_read_humi(temphumi_drv_t * dev, float *humi)
{
    //读取湿度
}
static void temphumi_drv_read_temp_and_humi(temphumi_drv_t * dev, float *temp, float *humi)
{
    EventBits_t uxBits;
    const EventBits_t uxBitsToWaitFor = EVENT_TEMPHUMI;
    (void)dev;
    //读取温度
    temp_humi_xxx_event_t event = {
        // .temperature = &temperature,
        // .humidity = &humidity,
        .lifetime = 10000,
        // .timestamp = input_arg->timebase_interface->pfget_tick_count,
        .type = TEMP_HUMI_EVENT_TEMP,
        .pfcallback = temp_humi_callback
    };
    bsp_temp_humi_xxx_read(&event);
    //事件唤醒
    uxBits = xEventGroupWaitBits(
            xtemphumi_event_flags_handle,        // 事件组句柄
            uxBitsToWaitFor,    // 要等待的事件位
            pdTRUE,             // 退出时清除事件位
            pdTRUE,             // 所有事件位都要设置
            portMAX_DELAY       // 无限期等待
        );
    if ((uxBits & uxBitsToWaitFor) == uxBitsToWaitFor) 
    {
        *temp = s_temperature;
        *humi = s_humi;
    }
}

static void temp_humi_callback(float *temperature, float *humidity)
{
    log_d("callback:temperature = %f, humidity = %f", *temperature, *humidity);
    
    //Forbiden :#1035-D: single-precision operand 
    //implicitly converted to double-precision so with (float)1.5
    s_temperature = (*temperature)*(float)1.5;
    s_humi = *humidity;
    xEventGroupSetBits(xtemphumi_event_flags_handle, EVENT_TEMPHUMI);
}


//******************************** Functions ********************************//
