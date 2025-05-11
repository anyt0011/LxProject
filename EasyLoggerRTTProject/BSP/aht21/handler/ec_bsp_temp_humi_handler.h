/*
 * Copyright (C) 2024 EternalChip Co.，Ltd. or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * File name: ec_aht21_handler.h
 * 
 * Description: Provide the APIs of aht21 and execute the event that APIs 
 *              cause.
 * 
 * Version: V1.1
 * 
 * Modifications:
 * V1.0: Complete the basic functions and call them according to 
		 their priority; 
 * V1.1: Make some changes on format; 
 * 
 * Note: 1 tab == 4 spaces!
 * 
 */

#ifndef __EC_TEMP_HUMI_HANDLER_H__
#define __EC_TEMP_HUMI_HANDLER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct {
    //Core层提供的接口
    iic_driver_interface_t *iic_driver_interface_table;            //IIC的实体实例
    system_timebase_interface_t * timebase;                                //时基
    //RTOS提供的接口
    void (*pf_rtos_yeild)(uint32_t delay_time);                    // 操作系统切换

} thread_input_paras_t;

typedef struct {

    // 1.RTOS related
    uint8_t inited;
    void * lock;
    void * handler_event_queue;
    void * handler_thread;
    
    // 2.variables used by thread
    uint32_t * pv_temp_humi_lifetime;
    uint32_t * pv_temp_humi_timestamp;
    float * pv_temp_humi_temp;
    float * pv_temp_humi_humi;

} temp_humi_inner_var_t;

typedef struct {
    //外部提供的接口
    thread_input_paras_t                    input_parameters;

    //handler维护的变量
    void * pv_temp_humi_hal_instance;                 //底层驱动实例


    //handler自有函数
    int8_t (*pf_inst)(temp_humi_handler_t 
                                    *ps_temp_humi_handler); // #aht21 handler实例
    int8_t (*pf_deinst)(temp_humi_handler_t 
                                    *ps_temp_humi_handler); // 操作系统切换       
    int8_t (*pf_init)  (temp_humi_handler_t 
                                    *ps_temp_humi_handler);    //初始化
	int8_t (*pf_deinit)(temp_humi_handler_t 
                                    *ps_temp_humi_handler);    //逆初始化
}temp_humi_handler_t;

typedef enum {
    TEMP_HUMI_EVENT_TYPE_TEMP = 0,
    TEMP_HUMI_EVENT_TYPE_HUMI,
    TEMP_HUMI_EVENT_TYPE_BOTH,
}temp_humi_t;


typedef struct{
    float * temp;
    float * humi;
    uint32_t * lifetime;
    uint32_t * timestamp;
    temp_humi_t type_of_data;
    void (*callback)(float * , float *)
} temp_humi_event_t;

/**
 * @brief 构造temp_humi_Handler
 *
 * 这个函数构造temp_humi_handler_t的实例化对象以及相关的操作系统资源。
 *
 * @param handler AHT21 Handler 实例
 * @param aht21_instance AHT21传感器实例
 * @param iic_instance I2C驱动接口实例
 * @param timebase 系统时基接口实例
 * @param rtos_yeild 操作系统任务切换函数
 * 
 * @return 0 表示成功，其他值表示失败
 */

/***************************Functions_Internal********************************/
int8_t temp_humi_handler_inst(  temp_humi_handler_t *aht21_handler_instance, 
                                bsp_aht21_t *bsp_aht21_instance, //TBD 可导入hal_driver加载链表
                                iic_driver_interface_t *iic_instance,
                                system_timebase_interface_t *timebase, void *rtos_yeild);

/**
 * @brief 解构AHT21 Handler
 *
 * 这个函数初始化AHT21传感器实例以及相关的操作系统资源。
 *
 * @param handler AHT21 Handler 实例
 * @return 0 表示成功，其他值表示失败
 */
int8_t temp_humi_handler_deInst(temp_humi_handler_t *handler);

/**
 * @brief 初始化AHT21 Handler
 *
 * 这个函数初始化AHT21传感器实例以及相关的操作系统资源。
 *
 * @param handler AHT21 Handler 实例
 * @return 0 表示成功，其他值表示失败
 */
int8_t temp_humi_handler_init(temp_humi_handler_t *handler);

/**
 * @brief 逆初始化AHT21 Handler
 *
 * 这个函数初始化AHT21传感器实例以及相关的操作系统资源。
 *
 * @param handler AHT21 Handler 实例
 * @return 0 表示成功，其他值表示失败
 */
int8_t temp_humi_handler_deinit(temp_humi_handler_t *handler);
/***************************Functions_Internal********************************/


/***************************Functions_External********************************/
/**
 * @brief Function for runing the stuff of handler.
 *
 * @param[in] arg : Pointer to argument from OS;
 * 
 * @return  void.
 * 
 * */
void temp_humi_handler_thread(void * arg);

/**
 * @brief Function for runing the stuff of handler.
 *
 * @param[in] arg : Pointer to argument from OS;
 * 
 * @return  void.
 * 
 * */
int8_t temp_humi_event_handler_send( temp_humi_event_t * event);

/***************************Functions_External********************************/

#endif //__EC_TEMP_HUMI_HANDLER_H__
