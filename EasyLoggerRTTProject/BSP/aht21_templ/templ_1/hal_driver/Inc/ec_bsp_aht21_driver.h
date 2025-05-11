/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file ec_bsp_aht21_driver.h
 * 
 * @par dependencies 
 * - ec_bsp_aht21_reg.h
 * - stdio.h
 * - stdint.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
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

#ifndef __EC_BSP_AHT21_DRIVER_H__  //Avoid repeated including same files later
#define __EC_BSP_AHT21_DRIVER_H__

//******************************** Includes *********************************//
#include "ec_bsp_aht21_reg.h"      // 包含自己的寄存器.h文件一定是在最开始

#include <stdio.h>                    // 编译器提供的库、用尖括号
#include <stdint.h>

//******************************** Includes *********************************//


//******************************** Defines **********************************//
#define OS_MULTICORE_SUPPORTING 
#define DEBUG

//******************************** Defines **********************************//
//******************************** Declaring ********************************//
//Interfaces from Core
// 1. I2C interface
typedef struct
{
    int8_t  (*pf_init         ) (void);
    int8_t  (*pf_deinit       ) (void);
    int8_t  (*pf_start        ) (void);
    int8_t  (*pf_stop         ) (void);
    int8_t  (*pf_wait_ack     ) (void);
    int8_t  (*pf_send_ack     ) (void);
    int8_t  (*pf_send_not_ack ) (void);
    int8_t  (*pf_send_bytes   ) (uint8_t addr, 
                                uint8_t *pdata, 
                                uint8_t size);
    int8_t  (*pf_receive_bytes) (uint8_t addr, 
                                uint8_t *pdata, 
                                uint8_t size);
} iic_driver_interface_t;

// 2. Timebase interface
typedef struct 
{
    int32_t (*pfget_tick_count) (void);                           //系统的时间
} timebase_interface_t;

//Interfaces from OS
// 3. RTOS yield interface
#ifdef OS_MULTICORE_SUPPORTING 

typedef struct 
{
    void (*rtos_yield) (void);                                   //当前线程让出CPU
} yield_interface_t;

#endif // OS_SUPPORTING

// 4.irq interface
typedef struct 
{

#ifdef OS_SUPPORTING 
    int8_t (*pflock) (void);                  // #1 加线程锁
    int8_t (*pfunlock) (void);                // #2 解线程锁
#endif // OS_SUPPORTING

    int8_t (*pfdisable_irq) (void);           // #3 关中断
    int8_t (*pfenable_irq) (void);            // #4 释放中断
} irq_interface_t;

typedef struct bsp_aht21_driver_t {

    // 外部提供的接口
    iic_driver_interface_t         *piicdriver_interface;         // IIC接口
    timebase_interface_t            *ptimebase_interface;         // 时基接口
    yield_interface_t              *rtos_yield_interface;         //操作系统让出CPU接口
    irq_interface_t                       *irq_interface;         //中断保护接口
    
    // iic 实例
    void                                  *piic_instance;         // IIC的实体实例

    // 通过构造函数来挂载外部需要的接口给aht21_instance
    int8_t (*pfinst)(                                             // #0 构造函数
                    struct bsp_aht21_driver_t*    aht21_instance, // AHT21的实体实例
    
                    iic_driver_interface_t *      iic_instance,   // IIC的实体实例
                    
                    timebase_interface_t *        timebase,       // 时基
#ifdef OS_SUPPORTING
                    yield_interface_t *           rtos_yield,     //让出CPU函数
#endif                                        
                    irq_interface_t *             irq_interface   //中断屏蔽
                    );// 操作系统切换
    
    int8_t (*pfdeinst)(struct bsp_aht21_driver_t * aht21_instance);// #0 去构造函数
    
    // aht21_hal应该实现的HAL接口
    int8_t (*pfinit)(struct bsp_aht21_driver_t * aht21_instance);        // #1 去初始化函数
    int8_t (*pfdeinit)(struct bsp_aht21_driver_t * aht21_instance);      // #2 去初始化函数
    int8_t (*pfread_id)(struct bsp_aht21_driver_t * aht21_instance);     // #3 读取ID函数
    int8_t (*pfread_temperature)(struct bsp_aht21_driver_t * aht21_instance,
                                                        float *temp);    // #4 读温度
    int8_t (*pfread_humidity)(struct bsp_aht21_driver_t * aht21_instance,
                                                        float *humi);    // #5 读湿度
    int8_t (*pfhibernating)(struct bsp_aht21_driver_t * aht21_instance); // #6 休眠
    int8_t (*pfwakeup)(struct bsp_aht21_driver_t * aht21_instance);      // #7 唤醒
    
}bsp_aht21_driver_t;
 
//******************************** Declaring ********************************//

// #0 构造函数
int8_t aht21_inst(
                bsp_aht21_driver_t *           aht21_instance,    // AHT21的实体实例
                iic_driver_interface_t *      iic_instance,       // IIC的实体实例
                   
                timebase_interface_t *        timebase,           // 时基
#ifdef OS_SUPPORTING
                yield_interface_t *           rtos_yield,         //操作系统最小让出CPU
#endif
                irq_interface_t *             irq_interface       //中断接口
                ); // 操作系统切换
                

//******************************** Declaring ********************************//

#endif //__EC_BSP_AHT21_DRIVER_H_
