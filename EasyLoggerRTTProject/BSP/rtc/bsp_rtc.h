/**
  ******************************************************************************
  * File Name          : bsp_rtc.h
  * Description        : 
  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */

#ifndef __BSP_RTC_H_
#define __BSP_RTC_H_


/* Private includes ----------------------------------------------------------*/


/* 底层HAL头文件包含 */
#include "main.h"
#include "stdbool.h"
//#include "rtc.h"
//#include "gpio.h"

/* OS头文件包含 */


/* 板级支持包头文件包含 */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char date;
    unsigned char week;
    unsigned char month;
    unsigned short year;
} TIME_FMT;
/* Private define ------------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/
#define STARTOFTIME             ( 1970U )
#define SECDAY                  ( 86400UL )
#define leapyear(year)          ((year) % 4U == 0U )
#define days_in_year(y)         (leapyear(y) ? (unsigned long)366U : (unsigned long)365U)
#define days_in_month(m)        (month_days[(m) - 1])

/* Private variables ---------------------------------------------------------*/

void bsp_rtc_init(void);

uint32_t bsp_mktime(TIME_FMT *time);
void bsp_gmtime(uint32_t tim,TIME_FMT *tm);
static void updateTimestamp(TIME_FMT *tm);
static void recvTimestamp(TIME_FMT *tm);
uint32_t bsp_time(TIME_FMT *tm);
bool bsp_rtc_SetSystemTime(char *timer);
void bsp_rtc_GetSystemTime(char *timer);
void bsp_rtc_GetSystemTime_Fmt(char *timer);
uint32_t bsp_get_interval_by_tick(uint32_t last_tick, uint32_t now_tick);
uint8_t bsp_rtc_TimeToBCD (TIME_FMT rtTime, uint8_t *p_dst);
bool bsp_rtc_BCDToRtcTime(TIME_FMT *rtTime, uint8_t *p_src );

#endif /*__BSP_RTC_H_*/



