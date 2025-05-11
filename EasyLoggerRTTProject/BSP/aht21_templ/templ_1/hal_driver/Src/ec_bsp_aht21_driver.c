/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file ec_bsp_aht21_driver.c
 * 
 * @par dependencies 
 * - ec_bsp_aht21_driver.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Implete the HAL operations of AHT21 and corresponding opetions.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0
 * 
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/

#include "ec_bsp_aht21_driver.h"

/******************************************************************************
  * @name    aht21_inst
  * @brief   instancetiate the AHT21 instance
  * @param   aht21_instance[in]
  * @param   piicdriver_instance[in]
  * @param   time_base_instance[in]
  * @param   rtos_yeild[in]
  * 
  * @return 0 success
  *            -1 aht21_instance null
  *         -2 iic_instance null
  *         -3 timebase null
  *         -4 rtos_yeild null
  *         -5 AHT21 ADDR error
 *****************************************************************************/
static int8_t aht21_init(bsp_aht21_driver_t * aht21_instance);  // #1 初始化函数
static int8_t aht21_deinit(bsp_aht21_driver_t * aht21_instance);  // #2 去初始化函数
static int8_t aht21_read_id(bsp_aht21_driver_t *aht21_instance);   // #3 读取ID函数
                                                                   
static int8_t aht21_read_temperature(bsp_aht21_driver_t *aht21_instance, 
                                                    float *temp);     // #4 读温度
static int8_t aht21_read_humidity(bsp_aht21_driver_t *aht21_instance, 
                                                    float *temp);        // #5 读湿度
static int8_t aht21_hibernating(bsp_aht21_driver_t *aht21_instance);     // #6 休眠
static int8_t aht21_wakeup(bsp_aht21_driver_t *aht21_instance);         // #7 唤醒

int8_t aht21_inst(
                bsp_aht21_driver_t *           aht21_instance,     // AHT21的实体实例
                iic_driver_interface_t *      iic_instance,        // IIC的实体实例
                timebase_interface_t *        timebase,            // 时基
#ifdef OS_SUPPORTING
                yield_interface_t *           rtos_yield,          //操作系统最小让出CPU
#endif
								irq_interface_t *             irq_interface        //中断接口
                )
{
    //0. Check the input parameters is NULL or not
    if (NULL == aht21_instance)
    {
        return -1;
    }
    if (NULL == iic_instance)
    {
        return -2;
    }
    if (NULL == timebase)
    {
        return -3;
    }
#ifdef OS_SUPPORTING
    if (NULL == rtos_yield)
    {
        return -4;
    }
#endif
    //1.Load the iic_driver_interface
    aht21_instance->piicdriver_interface = iic_instance;
    
    if (NULL == aht21_instance->piicdriver_interface->pf_init         || 
        NULL == aht21_instance->piicdriver_interface->pf_deinit       ||
        NULL == aht21_instance->piicdriver_interface->pf_start        ||
        NULL == aht21_instance->piicdriver_interface->pf_stop         ||
        NULL == aht21_instance->piicdriver_interface->pf_wait_ack     ||
        NULL == aht21_instance->piicdriver_interface->pf_send_ack     ||
        NULL == aht21_instance->piicdriver_interface->pf_send_not_ack ||
        NULL == aht21_instance->piicdriver_interface->pf_send_bytes   ||
        NULL == aht21_instance->piicdriver_interface->pf_receive_bytes||
        NULL == aht21_instance->piicdriver_interface->pf_send_bytes
        )
    {
        //deinstantiate the aht21_instance
        aht21_instance->pfdeinst(aht21_instance);
        return -2;
    }

    //2.Load the time_base_instance

    aht21_instance->ptimebase_interface = timebase;

    if( NULL == timebase->pfget_tick_count )
    {
        //deinstantiate the aht21_instance
        aht21_instance->pfdeinst(aht21_instance);
        return -3;
    }

    //3.Load teh rtos_yeild
#ifdef OS_SUPPORTING
    aht21_instance->rtos_yield_interface = rtos_yield;
    if(NULL == aht21_instance->rtos_yield_interface->rtos_yield )
    {
        //TBD: 自适应RTOS延时接口

        //deinstantiate the aht21_instance
        aht21_instance->pfdeinst(aht21_instance);
        return -4;
    }
#endif //OS_SUPPORTING
        
    //4.Load irq_interface_t 
    aht21_instance->irq_interface = irq_interface;
    if(NULL == aht21_instance->irq_interface )
    {
        //TBD: 自适应RTOS延时接口

        //deinstantiate the aht21_instance
        aht21_instance->pfdeinst(aht21_instance);
        return -4;
    }

    // 5.Initialization
    aht21_instance->pfinit = aht21_init;
    aht21_instance->pfread_id = aht21_read_id;
    if(0x38 != aht21_read_id(aht21_instance)){
        aht21_instance->pfdeinst(aht21_instance);
        return -5;
    }
    aht21_instance->pfdeinit =                     aht21_deinit;
    aht21_instance->pfread_humidity =       aht21_read_humidity;
    aht21_instance->pfread_temperature = aht21_read_temperature;
    aht21_instance->pfhibernating =           aht21_hibernating;
    aht21_instance->pfwakeup =                     aht21_wakeup;

    return 0;
}


/**
  * @Name    aht21_init
  * @brief  
  * @param   aht21_instance[in]
  * @retval  -1 error
  *          0  ok
  * @Data    2024-05-30
 **/
int8_t aht21_init(bsp_aht21_driver_t *aht21_instance)
{
    // 1.IIC initialization
    aht21_instance->piicdriver_interface->pf_init();
    // 2.Aht21_init
    //delay 10ms
    int32_t count = aht21_instance->ptimebase_interface->pfget_tick_count();
    while ( 
        (aht21_instance->ptimebase_interface->pfget_tick_count() - count) < 40)
    {
#ifdef OS_MULTICORE_SUPPORTING
        aht21_instance->rtos_yield_interface->rtos_yield();
#endif //OS_SUPPORTING
    }
    // 3.read 0x71 to check it it is ready with bit[3] is set to 1;
    uint8_t read_data = 0;
    aht21_instance->piicdriver_interface->pf_receive_bytes(0x71, &read_data, 1);
    if (0x08!= (read_data & 0x08))
    {
        aht21_instance->piicdriver_interface->pf_receive_bytes(0x71, &read_data, 1);
        //delay 10ms
        int32_t count = aht21_instance->
                                    ptimebase_interface->pfget_tick_count();
        while ( 
                (aht21_instance->
                    ptimebase_interface->pfget_tick_count() - count) < 10
                                                                           )
        {
#ifdef OS_SUPPORTING
            aht21_instance->rtos_yield_interface->rtos_yield();
#endif //OS_SUPPORTING
        }
    }
    return 0;
}


/**
  * @Name    aht21_deinit
  * @brief  
  * @param   aht21_instance[in]
  * @retval  -1 error
  *          0  ok
  * @Data    2024-05-30
 **/
int8_t aht21_deinit(bsp_aht21_driver_t *aht21_instance)
{
    aht21_instance->piicdriver_interface->pf_deinit();
    return 0;
}

/**
 * @brief  ATH21 解构
 *
 * @param  *ath21_instance  aht21实例
 *
 * @return 0 success
 *
 */
int8_t aht21_deInst(bsp_aht21_driver_t *aht21_instance){
    if (aht21_instance != NULL)
    {
        //AHT21实例IIC逆初始化
        if (aht21_instance->piicdriver_interface!=NULL)
        {
            aht21_instance->piicdriver_interface->pf_deinit = NULL;
            aht21_instance->piicdriver_interface->pf_receive_bytes = NULL;
            aht21_instance->piicdriver_interface->pf_send_bytes = NULL;
            aht21_instance->piicdriver_interface->pf_init = NULL;
            aht21_instance->piicdriver_interface = NULL;
        }
        //AHT21实例时基单元逆初始化
        if(aht21_instance->ptimebase_interface != NULL){
            aht21_instance->ptimebase_interface->pfget_tick_count=NULL;
        }
        //AHT21实例逆初始化
        aht21_instance->pfdeinit =           NULL;
        aht21_instance->pfread_humidity =    NULL;
        aht21_instance->pfread_temperature = NULL;
        aht21_instance->pfhibernating =      NULL;
        aht21_instance->pfwakeup =           NULL;
    }
    return 0;
}

/**
  * @Name    aht21_read_id
  * @brief  
  * @param   aht21_instance[in]
  * @retval  -1 error
  *          other: id
  * @Data    2024-05-30
 **/
int8_t aht21_read_id(bsp_aht21_driver_t *aht21_instance)
{
    return 0;
}
/**
  * @Name    aht21_read_status
  * @brief  
  * @param   aht21_instance[in]
  * @retval  1 busy
  *          0 idle
  * @Data    2024-05-30
 **/
int8_t aht21_read_status(bsp_aht21_driver_t *aht21_instance)
{
    uint8_t write_data[3] ={AHT21_REG_POINTER_AC,AHT21_AC_1,AHT21_AC_2};
    aht21_instance->piicdriver_interface->pf_send_bytes(AHT21_IIC_ADDR,write_data,3);

    uint8_t read_buff[7] ={0};
    aht21_instance->piicdriver_interface->pf_receive_bytes(AHT21_IIC_ADDR,read_buff,7);

    return (read_buff[0] & 0x80);  
}
/**
  * @Name    aht21_read_temperature
  * @brief  
  * @param   aht21_instance[in]
  *          temperature [out]
  * @retval  -1 error
  *          0 ok
  * @Data    2024-05-30
 **/
int8_t aht21_read_temperature(bsp_aht21_driver_t *aht21_instance,float *temperature)
{
    uint8_t write_data[3] ={AHT21_REG_POINTER_AC,AHT21_AC_1,AHT21_AC_2};
    aht21_instance->piicdriver_interface->pf_send_bytes(AHT21_IIC_ADDR,write_data,3);

    uint8_t read_buff[7] ={0};
    aht21_instance->piicdriver_interface->pf_receive_bytes(AHT21_IIC_ADDR,read_buff,7);

    if(0x00 == (read_buff[0] & 0x80)){
        uint32_t temp = 0;
        temp = (temp | read_buff[3])<<8;
        temp = (temp | read_buff[4])<<8;
        temp = (temp | read_buff[5]);
        temp = temp & 0x000fffff;
        *temperature = (float)(temp*200.0/1024.0/1024.0-50.0);
        return 0;
    }
    else{
        return -1;
    }
}
/**
  * @Name    aht21_read_humidity
  * @brief  
  * @param   aht21_instance[in]
  *          humidity [out]
  * @retval  -1 error
  *          0 ok
  * @Data    2024-05-30
 **/
int8_t aht21_read_humidity(bsp_aht21_driver_t *aht21_instance,float *humidity)
{
    uint8_t write_data[3] ={AHT21_REG_POINTER_AC,AHT21_AC_1,AHT21_AC_2};
    aht21_instance->piicdriver_interface->pf_send_bytes(AHT21_IIC_ADDR,write_data,3);

    uint8_t read_buff[7] ={0};
    aht21_instance->piicdriver_interface->pf_receive_bytes(AHT21_IIC_ADDR,read_buff,7);

    if(0x00 == (read_buff[0] & 0x80)){
        uint32_t humi = 0;
        humi = (humi | read_buff[1])<<8;
        humi = (humi | read_buff[2])<<8;
        humi = (humi | read_buff[3]);
        *humidity = (float)(humi*100.0/1024.0/1024.0);
        return 0;
    }
    else{
        return -1;
    }
}
/**
  * @Name    aht21_hibernating
  * @brief  
  * @param   aht21_instance[in]
  * @retval  -1 error
  *          0 ok
  * @Data    2024-05-30
 **/
int8_t aht21_hibernating(bsp_aht21_driver_t *aht21_instance)
{
    return 0;
}
/**
  * @Name    aht21_wakeup
  * @brief  
  * @param   aht21_instance[in]
  * @retval  -1 error
  *          0 ok
  * @Data    2024-05-30
 **/
int8_t aht21_wakeup(bsp_aht21_driver_t *aht21_instance)
{
    return 0;
}
