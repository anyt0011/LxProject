/**
  ******************************************************************************
  * File Name          : bsp_rtc.c
  * Description        : 
  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */

#define LOG_TAG      "bsp_rtc.c"

/* Private includes ----------------------------------------------------------*/
/* C头文件包含 */
#include <stdio.h>
#include <string.h>
#include <time.h>
/* 底层HAL头文件包含 */
#include "main.h"
#include "rtc.h"
/* OS头文件包含 */
#include "FreeRTOS.h"

/* 三方库头文件包含 */
#include "elog.h"
#include "bsp_rtc.h"
#include "bcd_hex_converter.h"
/* Private typedef -----------------------------------------------------------*/
//RTC_HandleTypeDef hrtc;

/* Private define ------------------------------------------------------------*/
#define  INITED             1
#define  NOT_INITED         0


/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
static uint8_t rtc_inited = NOT_INITED;
static int64_t sys_sec_Time;
/* Private function prototypes -----------------------------------------------*/




bool is_valid_time(const TIME_FMT *time) {
    if (time == NULL) return false;
    if (time->month < 1 || time->month > 12) return false;
    if (time->date < 1 || time->date > 31) return false;
    if (time->hour > 23) return false;
    if (time->minute > 59) return false;
    if (time->second > 59) return false;
    return true;
}

/* 
 * 函数功能：将公历日期转换为自1970-01-01 00:00:00以来的秒数。
 * 输入参数：
 *   time - 指向TIME_FMT结构体的指针，包含年、月、日、时、分、秒信息。
 * 返回值：
 *   自1970-01-01 00:00:00以来的秒数，类型为uint32_t。
 *
 * 注意事项：
 *   1. 假设输入为标准日期格式，例如：1980-12-31 23:59:59 对应 year=1980, mon=12, day=31, hour=23, min=59, sec=59。
 *   2. 对于使用儒略历的地区（如1917年前的俄罗斯，1752年前的英国及其殖民地，1582年前的其他地区），需要调整算法。
 *   3. 该算法最早由高斯提出。
 *   4. 警告：在32位系统上，此函数将在2106-02-07 06:28:16发生溢出！由于time_t是有符号类型，因此在2038-01-19 03:14:08也会出现问题。
 */

uint32_t bsp_mktime(TIME_FMT *time)
{
    // 检查输入时间的有效性
    if (!is_valid_time(time))
    {
        return 0; // 对于简单起见，返回0表示无效输入
    }

    // 创建一个临时时间结构以避免修改原始输入
    TIME_FMT Tmp_tm = *time;

    // 调整月份和年份以适应算法
    if (Tmp_tm.month > 2U)
    {
        Tmp_tm.month -= 2U;
    }
    else
    {
        Tmp_tm.month += 10U;
        Tmp_tm.year -= 1U;
    }

    // 计算临时变量temp，用于后续计算
    uint32_t temp = (uint32_t)Tmp_tm.year / 4U - 
                         (uint32_t)Tmp_tm.year / 100U + 
                         (uint32_t)Tmp_tm.year / 400U + 
                          367U * (uint32_t)Tmp_tm.month / 
                          12U + (uint32_t)Tmp_tm.date;

    // 计算并返回自1970年1月1日00:00:00 UTC以来的秒数
    return (((temp + (uint32_t)Tmp_tm.year * 365U - 719499U) 
            * 24U + Tmp_tm.hour) 
            * 60U + Tmp_tm.minute) 
            * 60U + Tmp_tm.second;
}

/**
 * 将给定的时间（从纪元开始的秒数）转换为日历日期和时间。
 *
 * @param tim  输入参数，表示从纪元开始经过的秒数。
 * @param tm   输出参数，指向 TIME_FMT 结构体的指针，用于存储转换后的日期和时间信息。
 */

void bsp_gmtime(uint32_t tim,TIME_FMT *tm)
{
    // 定义每个月的天数数组（非闰年）
    uint8_t month_days[12] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint32_t i, day, hms;

    // 计算总天数和剩余的小时、分钟、秒数
    day = tim / SECDAY;  // 总天数
    hms = tim % SECDAY;  // 剩余的秒数

    // 计算星期几（假设纪元开始是星期四）
    tm->week = (uint8_t)((day + 4U) % 7U);

    // 计算小时、分钟和秒数
    tm->hour = (uint8_t)(hms / 3600U);          // 小时
    tm->minute = (uint8_t)((hms % 3600U) / 60U); // 分钟
    tm->second = (uint8_t)((hms % 3600U) % 60U); // 秒

    // 计算年份
    for (i = (uint32_t)STARTOFTIME; day >= days_in_year(i); i++)
    {
        day -= (uint32_t)(days_in_year(i));
    }
    tm->year = (uint16_t)i;

    // 如果是闰年，则2月有29天
    if (leapyear(tm->year))
    {
        days_in_month(2) = (uint8_t)29U;
    }

    // 计算月份
    for (i = 1U; day >= days_in_month((uint8_t)i); i++)
    {
        day -= days_in_month((uint8_t)i);
    }
    tm->month = (uint8_t)i;

    // 计算日期（剩余的天数加1）
    tm->date = (uint8_t)(day + 1U);
}


/**
 * @brief 更新实时时钟(RTC)的时间和日期
 * 
 * 该函数通过HAL库设置RTC的时间和日期，确保硬件时间与给定的时间结构体同步
 * 使用STM32 HAL库操作RTC寄存器，以二进制格式更新时间和日期
 * 
 * @param tm 指向时间结构体的指针，该结构体包含需要设置的时、分、秒、年、月、日、星期信息
 */
static void updateTimestamp(TIME_FMT *tm)
{
    // 初始化时间结构体
    RTC_TimeTypeDef sTime = {0};
    // 初始化日期结构体
    RTC_DateTypeDef sDate = {0};
    
    // 将时间结构体中的时、分、秒赋值给RTC时间结构体
    sTime.Hours = tm->hour;
    sTime.Minutes = tm->minute;
    sTime.Seconds = tm->second;
    // 使用HAL库函数设置RTC时间，如果设置失败，则调用错误处理函数
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 将时间结构体中的年、月、日、星期赋值给RTC日期结构体，年份需要减去2000以符合RTC的年份表示方式
    sDate.WeekDay = tm->week;
    sDate.Month = tm->month;
    sDate.Date = tm->date;
    sDate.Year = tm->year - 2000;
    // 使用HAL库函数设置RTC日期，如果设置失败，则调用错误处理函数
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
    
    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x5050);
    
    char systimer[15];
    bsp_rtc_GetSystemTime_Fmt(systimer);
    log_d("Set System date:%s",systimer);
}


/**
 * @brief 接收时间戳并将其存储到指定的时间结构体中
 * 
 * 本函数从实时时钟(RTC)模块获取当前时间和日期，并将其存储到传入的TIME_FMT结构体指针tm中
 * 函数首先获取当前时间，然后获取当前日期，并分别将它们存储到tm中相应的位置
 * 
 * @param tm 指向TIME_FMT结构体的指针，用于存储接收到的时间戳
 */
static void recvTimestamp(TIME_FMT *tm)
{
    // 初始化RTC时间结构体
    RTC_TimeTypeDef sTime = {0};
    // 初始化RTC日期结构体
    RTC_DateTypeDef sDate = {0};
    
    TIME_FMT tmp_tm = {0};
       
    // 从RTC获取当前时间
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        // 如果获取时间失败，则调用错误处理函数
        Error_Handler();
    }
    
    // 将获取到的时间存储到tm中
    tmp_tm.hour = sTime.Hours;
    tmp_tm.minute = sTime.Minutes;
    tmp_tm.second = sTime.Seconds;
    
    // 从RTC获取当前日期
    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        // 如果获取日期失败，则调用错误处理函数
        Error_Handler();
    }
    // 将获取到的日期存储到tm中
    tmp_tm.week = sDate.WeekDay;
    tmp_tm.month = sDate.Month;
    tmp_tm.date = sDate.Date;
    tmp_tm.year = sDate.Year;
    
    if(is_valid_time(&tmp_tm) != true)
    {
        return;
    }
    
    tm->hour = tmp_tm.hour ;
    tm->minute = tmp_tm.minute ;
    tm->second = tmp_tm.second ;
    
    tm->week = tmp_tm.week ;
    tm->month = tmp_tm.month ;
    tm->date = tmp_tm.date ;
    tm->year = tmp_tm.year ;
 
}


/**
 * @brief 根据给定的时间结构体设置或获取系统时间
 * 
 * 该函数根据传入的TIME_FMT指针tm执行不同的操作流程。如果tm不为空，
 * 则将tm所指向的时间结构体转换为自1970年1月1日以来的秒数，并更新系统时间。
 * 如果tm为空，则从某个源（如网络）接收时间戳，将其转换为年份基于2000年的
 * 时间结构体，并计算出自1970年1月1日以来的秒数。
 * 
 * @param tm 指向TIME_FMT结构体的指针，用于设置或接收时间信息
 * @return unsigned int 返回自1970年1月1日以来的秒数
 */

uint32_t bsp_time(TIME_FMT *tm)
{
    // 检查传入的时间结构体指针是否为空
    if (tm != NULL) {
        // 将时间结构体转换为自1970年1月1日以来的秒数
        sys_sec_Time = bsp_mktime(tm);
        // 将秒数转换回时间结构体格式
        bsp_gmtime(sys_sec_Time, tm);
        // 更新时间戳
        updateTimestamp(tm);
    }
    else {
        // 如果传入的指针为空，创建一个本地时间结构体变量
        TIME_FMT tm8025;
        // 从某个源接收时间戳
        recvTimestamp(&tm8025);
        // 将年份基于2000年计算
        tm8025.year += 2000;
        // 将时间结构体转换为自1970年1月1日以来的秒数
        sys_sec_Time = bsp_mktime(&tm8025);
    }
    // 返回自1970年1月1日以来的秒数
    return sys_sec_Time;
}


/**
 * 设置系统时间
 * 
 * 此函数接收一个字符数组，验证其起始两个字符是否为0x32和0x30（即ASCII码的'2'和'0'），
 * 如果不是，则返回false。如果验证通过，函数将字符数组转换为时间格式（TIME_FMT），
 * 并调用x_Time函数来设置系统时间
 * 
 * @param timer 包含时间信息的字符数组，格式应为"20YYMMDDHHMMSS"
 * @return bool 设置成功返回true，否则返回false
 */

bool bsp_rtc_SetSystemTime(char *timer)
{
    // 验证输入的时间字符串前两个字符是否为'2'和'0'，如果不是，则返回false
    if ((*timer != 0x32) || (timer[1] != 0x30)) {
        return false;
    }
    
    // 初始化一个缓冲区，用于存储从时间字符串转换得到的数字
    uint8_t numBuff[15] = {0};
    
    // 遍历时间字符串，将每个字符转换为数字并存储到numBuff中
    for (uint8_t i = 0; i < strlen(timer); i++)
    {
        numBuff[i] = timer[i] - 0x30;
    }
    
    // 初始化一个TIME_FMT结构体，用于设置系统时间
    TIME_FMT tm;
    
    // 将numBuff中的数字转换为年、月、日、时、分、秒，并赋值给tm结构体
    tm.year = (numBuff[0] * 1000) + (numBuff[1] * 100) + (numBuff[2] * 10) + numBuff[3];
    tm.month = numBuff[4] * 10 + numBuff[5];
    tm.date = numBuff[6] * 10 + numBuff[7];
    tm.hour = numBuff[8] * 10 + numBuff[9];
    tm.minute = numBuff[10] * 10 + numBuff[11];
    tm.second = numBuff[12] * 10 + numBuff[13];
    
    // 调用x_Time函数，将转换后的时间设置为系统时间
    bsp_time(&tm);
    
    // 返回true表示时间设置成功
    return true;
}


/**
 * 获取系统时间并格式化为字符串
 * 
 * 此函数的目的是将系统当前时间以yyyyMMddHHmmss的格式存储到传入的字符数组中
 * 它首先获取系统时间，然后将时间格式化为字符串，最后将该字符串复制到传入的字符数组中
 * 
 * @param timer 用于存储格式化后系统时间的字符数组
 */

void bsp_rtc_GetSystemTime(char *timer)
{
    // 定义一个时间结构体来存储系统时间
    TIME_FMT tm;
    
    // 将系统当前时间转换为时间结构体
    bsp_gmtime(bsp_time(NULL), &tm);
    
    // 定义一个字符数组用于存储格式化后的时间字符串，并初始化为0
    char Str[15];
    memset(Str, 0x00, sizeof(Str));
    
    // 将时间结构体中的时间信息格式化为字符串
    sprintf(Str, "%d%02d%02d%02d%02d%02d", tm.year, tm.month, tm.date, tm.hour, tm.minute, tm.second);
    
    // 将格式化后的时间字符串复制到传入的字符数组中
    strcpy(timer, Str);
}


void bsp_rtc_GetSystemTime_Fmt(char *timer)
{
        // 定义一个时间结构体来存储系统时间
    TIME_FMT tm;
    
    // 将系统当前时间转换为时间结构体
    bsp_gmtime(bsp_time(NULL), &tm);
    
    // 定义一个字符数组用于存储格式化后的时间字符串，并初始化为0
    char Str[15];
    memset(Str, 0x00, sizeof(Str));
    
    // 将时间结构体中的时间信息格式化为字符串
    sprintf(Str, "%d/%02d/%02d %02d:%02d:%02d", tm.year, tm.month, tm.date, tm.hour, tm.minute, tm.second);
    
    // 将格式化后的时间字符串复制到传入的字符数组中
    strcpy(timer, Str);
    
}


/**
 * 根据两次调用的时间戳计算时间间隔
 * 此函数考虑到了时间戳溢出的情况，因此可以在时间戳超过其数据类型限制时正确计算间隔
 * 
 * @param last_tick 上次事件发生的时间戳
 * @param now_tick 当前事件发生的时间戳
 * @return 返回两次事件之间的时间间隔
 */
uint32_t bsp_get_interval_by_tick(uint32_t last_tick, uint32_t now_tick)
{
  uint32_t interval = 0;
  // 如果当前时间戳大于等于上次时间戳，说明没有溢出，直接计算差值
  if (last_tick <= now_tick)
  {
      interval = now_tick  - last_tick;
  }
  else
  {
      // 如果当前时间戳小于上次时间戳，说明发生了溢出
      // 先将当前时间戳与上次时间戳之间的最大可能值（0xFFFFFFFF）相加，然后减去上次时间戳
      interval = now_tick + 0xFFFFFFFF - last_tick;
  }
  return interval;
}

/**
 * 将时间格式结构体中的时间转换为BCD编码格式
 * 
 * @param rtTime 包含时间信息的结构体，包括秒、分、小时、日期、星期、月和年
 * @param p_dst 指向用于存储BCD编码时间数据的缓冲区
 * @return 返回写入缓冲区的数据长度
 * 
 * 此函数按顺序将时间信息转换为BCD编码，并将其写入到p_dst指向的缓冲区中
 * BCD编码是一种二进制编码方式，用于表示十进制数字，这种方式在电子显示屏和串行通信中很常见
 * 选择这种编码方式可能是为了便于硬件处理或与其它系统兼容
 */

uint8_t bsp_rtc_TimeToBCD (TIME_FMT rtTime, uint8_t *p_dst)
{
    // 初始化缓冲区位置计数器
    uint8_t pos=0u;
    
    // 将秒、分、小时、日期、星期、月和年依次转换为BCD编码，并存储到缓冲区中
    p_dst[pos++] = HextoBCD(rtTime.second);
    p_dst[pos++] = HextoBCD(rtTime.minute);
    p_dst[pos++] = HextoBCD(rtTime.hour);
    p_dst[pos++] = HextoBCD(rtTime.date);
    
    // 星期需要特殊处理，将其左移5位，以便与其他字段进行位级联
    p_dst[pos]   = HextoBCD(rtTime.week)<<0x05;
    // 月份的BCD编码与星期的BCD编码合并存储，以节省空间
    p_dst[pos++] |=HextoBCD(rtTime.month);
    
    // 年份减去2000是为了只保存年份的后两位数字，适应BCD编码存储需求
    p_dst[pos++] = HextoBCD((uint8_t)(rtTime.year-2000u));
    
    // 返回写入缓冲区的数据长度
    return pos;
}


/**
 * 将BCD码转换为RTC时间格式
 * 
 * 此函数的目的是将二进制编码的十进制(BCD)格式的时间数据转换为RTC时间格式
 * 它用于将时间数据从一种紧凑的编码形式转换为更易于处理和读取的形式
 * 
 * @param rtTime 指向TIME_FMT结构体的指针，用于存储转换后的RTC时间
 * @param p_src 指向包含BCD编码时间数据的uint8_t数组的指针
 * 
 * @return bool 表示转换是否成功的布尔值 成功时返回true，否则返回false
 */
bool bsp_rtc_BCDToRtcTime(TIME_FMT *rtTime, uint8_t *p_src )
{
    // 初始化数组索引变量
    uint8_t pos =0u;
    
    // 检查输入参数的有效性
    if ((!rtTime) || (!p_src))
    {
      return false;
    }
    
    // 从BCD编码转换并赋值时间的各个部分
    rtTime->second = BCDtoHex(p_src[pos++]);
    rtTime->minute = BCDtoHex(p_src[pos++]);
    rtTime->hour = BCDtoHex(p_src[pos++]);
    rtTime->date = BCDtoHex(p_src[pos++]);
    
    // 对于星期，需要先右移5位再进行BCD到十进制的转换
    rtTime->week = BCDtoHex((p_src[pos])>>5);
    
    // 对于月份，先与0x1f进行与操作以提取有效位，再进行BCD到十进制的转换
    rtTime->month  = BCDtoHex(p_src[pos++] & 0x1fu);
    
    // 对于年份，转换后需要加上2000以得到实际的年份
    rtTime->year = BCDtoHex(p_src[pos++])+2000u;
    
    // 返回转换成功
    return true;
}



void bsp_rtc_init(void)
{ 
    if( INITED == rtc_inited )
    {
        return;
    }
    
    TIME_FMT tm = {0} ;  
    recvTimestamp(&tm);
    
    rtc_inited = INITED;

}


