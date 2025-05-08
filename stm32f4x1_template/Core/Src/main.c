/**
  ******************************************************************************
	WeAct 微行创新 
	>> 标准库实例例程
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define LOG_TAG "bootloader"

// 全局定义 STM32F411xE 或者 STM32F401xx
// 当前定义 STM32F411xE

// STM32F411 外部晶振25Mhz，考虑到USB使用，内部频率设置为96Mhz
// 需要100mhz,自行修改system_stm32f4xx.c

/** @addtogroup Template_Project
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t uwTimingDelay;
RCC_ClocksTypeDef RCC_Clocks;
uint8_t tab_1024[1029];

//下面的IV和Key就是你自定义的向量和密钥
// unsigned char IV[16]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};  
// unsigned char Key[32]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,\
//                        0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};
unsigned char IV[17]="1234123412341234";  
unsigned char Key[33]="12341234123412341234123412341234"; 
/* Private function prototypes -----------------------------------------------*/

#define RCC_OFFSET                 (RCC_BASE - PERIPH_BASE)
#define RCC_BDCR_OFFSET            (RCC_OFFSET + 0x70U)
#define RCC_RTCEN_BIT_NUMBER       0x0FU
#define RCC_BDCR_RTCEN_BB          (PERIPH_BB_BASE + (RCC_BDCR_OFFSET * 32U) + (RCC_RTCEN_BIT_NUMBER * 4U))
void system_deinit(void)
{
  /* Reset of all peripherals */
  (RCC->APB1RSTR = 0x10E2C80FU);
  (RCC->APB1RSTR = 0x00U);

  (RCC->APB2RSTR = 0x00177931U);
  (RCC->APB2RSTR = 0x00U);

  (RCC->AHB1RSTR = 0x0060109FU);
  (RCC->AHB1RSTR = 0x00U);

  (RCC->AHB2RSTR = 0x00000080U);
  (RCC->AHB2RSTR = 0x00U);

  (RCC->AHB3RSTR = 0xFFFFFFFFU);
  (RCC->AHB3RSTR = 0x00U) ;

  (*(__IO uint32_t *) RCC_BDCR_RTCEN_BB = DISABLE);

  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;
  /* De-Init the low level hardware */
  
}


/* Private functions ---------------------------------------------------------*/
 /**
  * @brief  Main program
  * @param  None
  * @retval None
  */
#define test
int main(void)
{
  // 配置系统时钟
  SystemInit();

  /* Init Systick */
  Delay_Init();

  /* Init Key */
  key_io_init();

  /* Init LED */
  led_io_init();

  /* Init USART */
  USART1_Init();

  /* Init Spi */
  SPI1_Init();

	/* Init W25Qx */
	W25Q64_Init();
	
  elog_init();
  /* set EasyLogger log format */
  elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
  elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
  elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
  elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
  elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
  elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
  /* start EasyLogger */
  elog_start();

  ee_CheckOk();


  uint8_t u8_ota_state_flag = 0xFF;
  uint8_t u8_app_state_flag = 0xFF;
  int32_t app_data_size = 0;
  int32_t old_app_data_size = 0;
  int32_t app_size = 0;
  uint8_t flag = 0;
  //16: Page2:App的状态
  ee_ReadBytes(&u8_app_state_flag,16,1);
	
//	 u8 * tmp = "111111111111111";
//	 uint8_t block_index = 1;
////	 Erase_Flash_Block(block_index);
//	 W25Q64_WriteData(tmp, 16, block_index);
//	 W25Q64_WriteData_End(block_index);
//	 
//	 u8 read[16]={0};
//	 uint16_t len = 0;
//	 W25Q64_ReadData(read, &len, block_index);
//	 log_i("len:%d, read:%s",len,read);
////	 Set_Block_Parameter(block_index,100);
//	 log_i("block%d size:%d",block_index,Read_Block_size(block_index));

#ifdef test
u8_app_state_flag = 0;
	if(1 == key_scan())
  {
		jump_to_app();
		u8_app_state_flag = 0;
	}
#endif
  if(0x01 == u8_app_state_flag)
  {
    //0：Page1：Ota的状态
    ee_ReadBytes(&u8_ota_state_flag,0,1);

    switch(u8_ota_state_flag)
    {
      case 0x00:
        if(1 == key_scan())
        {
          app_data_size = Ymodem_Receive(tab_1024);
          //Back flash to App flash
          //if(0 == Back_to_App_Flash(app_data_size))
          if(app_data_size > 0)
          {
            //把剩余数据也写进去 A区写完之后
            W25Q64_WriteData_End(BLOCK_1);
            //把数据解密后搬运到B区
            Write_ExternFlashB_After_AES_Decode(IV,Key);
            //把内部运行的App搬运到A区里面，搬运的是旧的App的Size
            ee_ReadBytes((uint8_t *)&old_app_data_size,24,4);
            Write_ExternFlashA_From_Flash(old_app_data_size);
            app_size = Write_Flash_From_ExternFlashB();
            //写入Page 0 为 0x00
            ee_WriteBytes(&flag,0,1);
            //写入Page 2 为 0x00
            ee_WriteBytes(&flag,16,1);
            //写入Page 3 为当前App的size
            ee_WriteBytes((uint8_t *)&app_size,24,4);
            jump_to_app();
          }
        }
        else
        {
          jump_to_app();
        }
      break;

      case 0x11:
          //直接进行跳转
          jump_to_app();
      break;

      case 0x22:
          ee_ReadBytes((uint8_t *)&app_data_size,8,4);
          Set_Block_Parameter(BLOCK_1,app_data_size);
          //把数据解密后搬运到B区
          Write_ExternFlashB_After_AES_Decode(IV,Key);
          //把内部运行的App搬运到A区里面，搬运的是旧的App的Size
          ee_ReadBytes((uint8_t *)&old_app_data_size,24,4);
          Write_ExternFlashA_From_Flash(old_app_data_size);
          app_size = Write_Flash_From_ExternFlashB();
          //写入Page 0 为 0x00
          ee_WriteBytes(&flag,0,1);
          //写入Page 2 为 0x00
          ee_WriteBytes(&flag,16,1);
          //写入Page 3 为当前App的size
          ee_WriteBytes((uint8_t *)&app_size,24,4);

					jump_to_app();
      break;

      case 0x33:
      //搬运旧数据回到内部flash里面
      ee_ReadBytes((uint8_t *)&app_data_size,8,4);
      Set_Block_Parameter(BLOCK_1,app_data_size);
      Write_Flash_From_ExternFlashA(app_data_size);
      u8_ota_state_flag = 0x00;
      ee_WriteBytes(&u8_ota_state_flag,0,1);
      jump_to_app();
      break;
      default:
      //No action
      break;
    }
  }
  else
  {
    app_data_size = Ymodem_Receive(tab_1024);
		log_i("ymodem receive size:%d", app_data_size);
    if(app_data_size > 0)
    {
      //把剩余数据也写进去 A区写完之后
      W25Q64_WriteData_End(BLOCK_1);
	    log_i("block1 size:%d",Read_Block_size(BLOCK_1));
      //把数据解密后搬运到B区
//			Erase_Flash_Block(BLOCK_2);
			log_i("block2 size:%d",Read_Block_size(BLOCK_2));
      Write_ExternFlashB_After_AES_Decode(IV,Key);
			log_i("block2 size:%d",Read_Block_size(BLOCK_2));
      //把B区数据搬运到内部flash
      app_size = Write_Flash_From_ExternFlashB();
			log_i("app size: %d", app_size);
      
      //写入Page 0 为 0x00
      ee_WriteBytes(&flag,0,1);
      //写入Page 2 为 0x00
      ee_WriteBytes(&flag,16,1);
      //写入Page 3 为当前App的size
      ee_WriteBytes((uint8_t *)&app_size,24,4);
      
      u8_ota_state_flag = 0x33;
      ee_WriteBytes(&u8_ota_state_flag,0,1);
      //新的App跳转
      IWDG_Init(IWDG_Prescaler_64,2000);
      jump_to_app();
    }
  }
#if 0 
  int32_t app_data_size = 0;
  if(1 == Key_Scan())
  {
    app_data_size = Ymodem_Receive(tab_1024);
    //Back flash to App flash
    //if(0 == Back_to_App_Flash(app_data_size))
    if(app_data_size > 0)
    {
      //把剩余数据也写进去
      W25Q64_WriteData_End();
      Write_ExternFlashB_After_AES_Decode(IV,Key);
      jump_to_app();
    }
  }
#endif
/*==================================================================================================*/
  //GPIO_Config();
  //TIM_Config();   
  /* Infinite loop */
  log_i("Not jump_to_application");

  while (1)
  {
    //Breathing_light();
		if(1 == key_scan())
		{
			app_data_size = Ymodem_Receive(tab_1024);
			//Back flash to App flash
			//if(0 == Back_to_App_Flash(app_data_size))
			if(app_data_size > 0)
			{
				//把剩余数据也写进去 A区写完之后
				W25Q64_WriteData_End(BLOCK_1);
				//把数据解密后搬运到B区
				Write_ExternFlashB_After_AES_Decode(IV,Key);
				//把内部运行的App搬运到A区里面，搬运的是旧的App的Size
				ee_ReadBytes((uint8_t *)&old_app_data_size,24,4);
				Write_ExternFlashA_From_Flash(old_app_data_size);
				app_size = Write_Flash_From_ExternFlashB();
				//写入Page 0 为 0x00
				ee_WriteBytes(&flag,0,1);
				//写入Page 2 为 0x00
				ee_WriteBytes(&flag,16,1);
				//写入Page 3 为当前App的size
				ee_WriteBytes((uint8_t *)&app_size,24,4);
				jump_to_app();
			}
		}
#if 0
    /* 测试Ymodem */
    if(1 == Key_Scan())
		  Ymodem_Receive(tab_1024);
    /* 测试串口 */
		if('a' == USART_ReceiveChar(USART1))
			USART_SendChar(USART1,'C');
    /*测试Flash*/
    if(EreaseAppSector(FLASH_Sector_6) == FLASH_COMPLETE)
    {
      Flash_Write(0x08040000,0x55555555);
    }
    /*测试Led*/
    Breathing_light();
#endif
	}
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
// void Delay(__IO uint32_t nTime)
// { 
//   uwTimingDelay = nTime;

//   while(uwTimingDelay != 0);
// }

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
// void TimingDelay_Decrement(void)
// {
//   if (uwTimingDelay != 0x00)
//   { 
//     uwTimingDelay--;
//   }
// }

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/