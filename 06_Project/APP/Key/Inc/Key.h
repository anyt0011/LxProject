/*
*******************************************************************
* Copyright (c) 2003-2020  TECHNOLOGY CO.,LTD.
* All Rights Coding Guideline for Programming Language C R00
Confidential Document - Reproduction, Disclosure and Circulation without authorization is prohibited
* Filename : Led.c
* Project : 06_Button_control_LED_light
* Description :
* Note(s) :
* Update Record:
* Version   Date          Modified by     Why & What is modified
* 00.00     YYYY.MM.DD    立芯嵌入式       Initial Version
*******************************************************************
*/
#ifndef __KEY_H
#define __KEY_H

#include "main.h"

// 定义按键端口和引脚
#define KEY_PORT GPIOA
#define KEY_PIN  GPIO_PIN_0

//按键滤波时间
#define KEY_FILTER_TIME             10      //MS

// 定义按键状态
typedef enum
{
    KEY_RELEASED = 0,
    KEY_PRESSED
} KeyState;

extern KeyState g_E_key_state;
extern void key_task_runnable(void);
#endif
