/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file Systim.h
 * 
 * @par dependencies 
 * - Systim.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide Bootloader Director Business Process
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
#ifndef __SYSTIM_H__
#define __SYSTIM_H__
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
extern void Delay_Init(void);

extern uint32_t millis(void)  ; //毫秒级运行定时器
extern uint32_t micros(void)  ; //微秒级运行定时器 

extern void delay_ms(uint32_t ms) ; //毫秒级延时函数 
extern void delay_us(uint32_t us) ; //毫秒级延时函数 


#endif
