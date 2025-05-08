/**
  ******************************************************************************
	WeAct 微行创新 
	>> 标准库实例例程
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "flash.h"
#include "usart.h"
#include "ymodem.h"

#define LOG_TAG "bootloader"
#include "elog.h"
#include "common.h"
#include "AES.h"
#include "W25Q_Handler.h"
#include "systim.h"
#include "spi.h"
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
unsigned char IV[16]="1234123412341234";  
unsigned char Key[32]="12341234123412341234123412341234"; 
/* Private function prototypes -----------------------------------------------*/
typedef void (*pf_app_t)(void);

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

void jump_to_app(uint32_t addr)
{
  uint32_t jump_address;
  pf_app_t jump_to_application;

  /* 检查栈顶地址是否合法 */
  if(((*(__IO uint32_t *)addr) & 0x2FFE0000) == 0x20000000)
  {
    /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
    __disable_irq();

    /* 用户代码区第二个 字 为程序开始地址(复位地址) */
    jump_address = *(__IO uint32_t *) (addr + 4);

    /* Initialize user application's Stack Pointer */
    /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
    __set_MSP(*(__IO uint32_t *) addr);

    /* 类型转换 */
    jump_to_application = (pf_app_t) jump_address;

    /* 跳转到 APP */
    jump_to_application();
  }
  return;
}

uint8_t back_to_app_flash(uint32_t App_size)
{
    uint8_t flash_state = Flash_erase(ApplicationAddress,App_size);
    if (1 == flash_state)
    {
        return 1;
    }
    uint32_t FlashDest = ApplicationAddress; 
    uint32_t Back_Source = (uint32_t)BackAppAddress;

    for (uint32_t j = 0;(j < App_size) && (FlashDest <  ApplicationAddress + App_size);j += 4)
    {
        Flash_Write(FlashDest,*(uint32_t*)Back_Source);
        //写完之后校验一下，是否写入
        if (*(uint32_t*)FlashDest != *(uint32_t*)Back_Source)
        {
            return 1;
        }
        FlashDest += 4;
        Back_Source += 4;
    }

    return 0;
}

/**********************Aes 加密 Test Start********************************/
uint8_t buf[16], temp_buf[16];
uint8_t aes256key[32] = "0123456789abcdef0123456789abcdef"; //密钥
uint8_t aes_iv[16] = "1212121212121212"; //向量

void print_hex(uint8_t *data, size_t len){
  for(size_t i=0;i<len;i++){
    elog_raw("%02x", data[i]);
  }
  elog_raw("\n");
}

void aes_test(void){
  char *string = "testtest";
  strcpy((char *)buf, string);
  buf[strlen(string)]='\0';
  log_i("------------Start-------------");
  log_i("data:%s",buf);
  elog_raw("data(hex):");
  print_hex(buf, 16);
  elog_raw("iv(hex):");
  print_hex(aes_iv, 16);
  strcpy(temp_buf, aes_iv);
  log_i("-----------Encrypt-----------");
  Aes_IV_key256bit_Encrypt(temp_buf,buf,aes256key);
  elog_raw("encrypt data(hex):");
  print_hex(buf, 16);
  elog_raw("encrypt iv(hex):");
  print_hex(temp_buf, 16);
  Aes_IV_key256bit_Decode(aes_iv,buf,aes256key);
  log_i("-----------Decode-----------");
  log_i("decode data:%s",buf);
  elog_raw("decode data(hex):");
  print_hex(buf, 16);
  log_i("------------End-------------");
}
/**********************Aes 加密 Test End********************************/

/**********************Aes 加密 Start********************************/
uint32_t BackSource;
//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void STMFLASH_Read_Byte(u32 ReadAddr,u8 *pBuffer,u16 NumToRead)   	
{
	while(NumToRead--)
	{
		*(pBuffer++) = *((vu8*)ReadAddr++) ;
	}
}

void Write_Flash_After_AES_Decode(u8 *IV_IN_OUT, u8 *key256bit)
{
    u8 Temp[16];  //原密文数据缓存
    u8 wirteTime=0;  //一个解析包写入次数
    u16 readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节）
    u16 writeTime=0,flashWriteCount=0;  //写入升级包的次数 （每次写入1024个字节）	
    u32 AppSize=0;  //升级包的大小
    u32 FlashReadAddr=BackAppAddress;
    u32 FlashDestination=ApplicationAddress;

    /*
      除了原有数据之外，首个16byte的数据是表达了文件的大小（自定义数据为8字节）
      自定数据长度（软件自动生成）| 自定义数据（>=8，且加8后能被16整除） | 加密前明文数据长度（软件自动生成）
      uin32_t类型，低位在前      |                                    | uin32_t类型，低位在前
    */
    STMFLASH_Read_Byte(FlashReadAddr,Temp,16);//读取头文件（包含自定义内容+升级包总字节）
    Aes_IV_key256bit_Decode(IV_IN_OUT,Temp,key256bit);//解析得到自定义内容+文件大小
    AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+Temp[12];
    log_d("AppSize=%d",AppSize);

    for(int i=0;i<16;i++){
      elog_raw("%02x,",Temp[i]);
    }
    elog_raw("\n");

    //计算升级包读取次数
    readDataCount=AppSize/16;
    if(AppSize%16!=0)
    {
      readDataCount+=1;
    }

    uint8_t flash_erase_state = Flash_erase(FlashDestination,AppSize);
    if(0 == flash_erase_state)
    {
      FlashReadAddr+=16;//偏移16个字节，偏移后，当前地址为升级包头字节
      FlashDestination=ApplicationAddress;

      for(uint16_t i=0;i<readDataCount;i++)
      {
        STMFLASH_Read_Byte(FlashReadAddr + i * 16,Temp,16);//读取头文件（包含自定义内容+升级包总字节）
        Aes_IV_key256bit_Decode(IV_IN_OUT,Temp,key256bit);//解析数据
        
        BackSource = (uint32_t)Temp;
        for (wirteTime = 0;wirteTime<4;wirteTime++)
        {
          Flash_Write(FlashDestination, *(uint32_t*)BackSource);
          FlashDestination += 4;
          BackSource += 4;
        }
      }
    }
}

/**********************Aes 加密 End********************************/
/* Private functions ---------------------------------------------------------*/
 /*
  *power by WeAct Studio
  *The board with `WeAct` Logo && `version number` is our board, quality guarantee. 
  *For more information please visit: https://github.com/WeActTC/MiniF4-STM32F4x1
  *更多信息请访问：https://gitee.com/WeActTC/MiniF4-STM32F4x1
  */
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	/* Enable Clock Security System(CSS): this will generate an NMI exception
     when HSE clock fails *****************************************************/
	SCB->VTOR=0x8000000 | 0x0;
  RCC_ClockSecuritySystemCmd(ENABLE);
	
 /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       files before to branch to application main.
       To reconfigure the default setting of SystemInit() function, 
       refer to system_stm32f4xx.c file */

  /* SysTick end of count event each 1ms */
  SystemCoreClockUpdate();
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
  
	
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
	
  /* Add your application code here */
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
	
  /* Insert 50 ms delay */
  Delay(50);
	log_i("cycle:%d",RCC_Clocks.HCLK_Frequency);
	aes_test();
	if(1 == key_scan())
  {
		log_i("waiting for app");
    int32_t app_data_size = Ymodem_Receive(tab_1024);
//    if(0 == back_to_app_flash(app_data_size)){
//      log_i("updata app succcess");
//    }else{
//      log_e("updata app failed");
//    }
		Write_Flash_After_AES_Decode(IV,Key);
  }
  else
  {
    log_i("bootloader not updata app");
  }
	
  log_i("system deinit");
  log_i("jump to app");
	system_deinit();
  jump_to_app(ApplicationAddress);

  while (1)
  {

		
	}
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{ 
  uwTimingDelay = nTime;

  while(uwTimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (uwTimingDelay != 0x00)
  { 
    uwTimingDelay--;
  }
}

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
