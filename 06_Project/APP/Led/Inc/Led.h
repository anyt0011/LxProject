/*
*******************************************************************
* Copyright (c) 2003-2020  TECHNOLOGY CO.,LTD.
* All Rights Coding Guideline for Programming Language C R00
Confidential Document - Reproduction, Disclosure and Circulation without authorization is prohibited
* Filename : Led.h
* Project : 06_Button_control_LED_light
* Description :Null
* Note(s) :
* Update Record:
* Version   Date          Modified by     Why & What is modified
* 00.00     YYYY.MM.DD    立芯嵌入式       Initial Version
*******************************************************************
*/
#ifndef LED_H
#define LED_H
#include "main.h"
#include "Key.h"

// 定义按键端口和引脚
#define LED_PORT GPIOC
#define LED_PIN  GPIO_Pin_13

#define LED_FLIP_EVENT_FILTER_TIME      100     //MS

//运行逻辑
extern void led_task_runnable(void);

#endif // LED_H
