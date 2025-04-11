/******************************************************************************
 * Copyright (C)
 *
 * All Rights Reserved.
 *
 * @file system_adaption.h
 *
 * @par dependencies
 * - stdio.h
 * - stdint.h
 *
 * @author Anyt
 *
 * @brief Integrate all the resources in the system and enable them to work.
 *
 * Processing flow:
 *
 * call directly.
 *
 * @version V1.0 2025-04-05
 *
 * @note 1 tab == 4 spaces!
 * @warning
 *****************************************************************************/
#ifndef __SYSTEM_ADAPTION_H__
#define __SYSTEM_ADAPTION_H__

//******************************* Includes **********************************//
#include "bsp_led_driver.h"
#include "bsp_led_handler.h"

#include <stdint.h>
#include <stdio.h>
//******************************* Includes **********************************//

//******************************** Defines **********************************//

typedef enum
{
    SYSTEM_OK              = 0,
    SYSTEM_ERROR           = 1,
    SYSTEM_ERROR_TIMEOUT   = 2,
    SYSTEM_ERROR_RESOURCES = 3,
    SYSTEM_ERROR_PARAMETER = 4,
    SYSTEM_ERROR_NOMEMORY  = 5,
    SYSTEM_ERROR_ISR       = 6,
    SYSTEM_ERROR_RESERVED  = 0XFF,
} system_status_t;

//******************************** Defines **********************************//

//******************************** Declaring ********************************//

void Test_1(void);
void Test_2(void);
void Test_3(void);

/**
 * @brief Init all the resources.
 *
 * Steps:
 *  1. mix up all the resources in this system.
 *
 * @param[in] self        :
 *
 * @return led_handler_status_t : Status of the function.
 *
 * */
led_status_t system_init_resources(void);

//******************************** Declaring ********************************//
#endif
