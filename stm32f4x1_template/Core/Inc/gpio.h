#ifndef _GPIO_H_
#define _GPIO_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#define LED_C13_PORT GPIOC
#define LED_C13_PIN  GPIO_Pin_13
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "main.h"
//#include "Systim.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

extern void key_io_init(void);
extern uint8_t key_scan(void);
extern void led_io_init(void);
extern void breathing_light(void);

#ifdef __cplusplus
}
#endif

#endif
