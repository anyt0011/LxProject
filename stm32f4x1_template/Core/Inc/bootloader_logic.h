/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file Bootloader_Logic.c
 * 
 * @par dependencies 
 * Bootloader_Logic.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2023-12-03
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
#ifndef __BOOTLOADER_LOGIC_H__  //Avoid repeated including same files later
#define __BOOTLOADER_LOGIC_H__

//******************************** Includes *********************************//
#include "main.h"

//******************************** Includes *********************************//

//******************************** Extern Function **************************//
void jump_to_app(void);
void STMFLASH_Read_Byte(u32 ReadAddr,u8 *pBuffer,u16 NumToRead);
void Write_ExternFlashB_After_AES_Decode(u8 *IV_IN_OUT, u8 *key256bit);
int32_t Write_Flash_From_ExternFlashB(void);
void Write_Flash_From_ExternFlashA(int32_t old_app_size);
//******************************** Extern Function **************************//

#endif

