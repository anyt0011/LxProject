/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file ec_bsp_aht21_reg.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the address of AHT21's registers.
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
#ifndef __EC_BSP_AHT21_REG_H__
#define __EC_BSP_AHT21_REG_H__

#define AHT21_IIC_ADDR	 		0x38
#define AHT21_STATUS            0x71

#define AHT21_IDLE		 		0x0
#define AHT21_BUSY				0x1

#define AHT21_REG_POINTER_AC 	0xAC
#define AHT21_AC_1 				0x33
#define AHT21_AC_2 				0x0

#endif //__EC_BSP_AHT21_REG_H__
