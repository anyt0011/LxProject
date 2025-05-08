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
/* Includes ------------------------------------------------------------------*/
#include "bootloader_logic.h"
/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* extern variables ---------------------------------------------------------*/

void jump_to_app(void)
{
  uint32_t JumpAddress;
  pFunction Jump_To_Application;

  /* 检查栈顶地址是否合法 */
  if(((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
  {
    //恢复默认
    USART_DeInit(USART1);
    /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
    __disable_irq();

    /* 用户代码区第二个 字 为程序开始地址(复位地址) */
    JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);

    /* Initialize user application's Stack Pointer */
    /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
    __set_MSP(*(__IO uint32_t *) ApplicationAddress);

    /* 类型转换 */
    Jump_To_Application = (pFunction) JumpAddress;

    /* 跳转到 APP */
    Jump_To_Application();
  }
}
/**********************Aes 加密 Start********************************/
//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数+
void STMFLASH_Read_Byte(u32 ReadAddr,u8 *pBuffer,u16 NumToRead)   	
{
	while(NumToRead--)
	{
		*(pBuffer++) = *((vu8*)ReadAddr++) ;
	}
}

extern uint32_t RamSource;
void Write_ExternFlashB_After_AES_Decode(u8 *IV_IN_OUT, u8 *key256bit)
{
    u8 Temp[16];  //原密文数据缓存
    u16 readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节）
    u32 AppSize=0;  //升级包的大小
    u8  Mem_Read_buffer[4096];
    u16 Read_Memory_Size=0;
    u32 Read_Memory_index=0;

    //先读一帧，用来解析头文件格式
    W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size,BLOCK_1);
	  log_d("Read_Memory_Size:%d",Read_Memory_Size);
    if(Read_Memory_Size  >= 16)
    {
      memcpy(Temp,Mem_Read_buffer,16);
      Aes_IV_key256bit_Decode(IV_IN_OUT,Temp,key256bit);//解析得到自定义内容+文件大小
      AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+Temp[12];
      log_d("AppSize=%d",AppSize);
			for(int i=0;i<16;i++){
				log_d("AppHead(%d):%02x",i,Temp[i]);
			}
      
      //计算升级包读取次数
      readDataCount=AppSize/16;
      if(AppSize%16!=0)
      {
        readDataCount+=1;
      }
      Read_Memory_index += 16;
    }

		log_i("1:%u",Read_Block_size(BLOCK_2));
		
    for(readTime=0;readTime<readDataCount;readTime++)
    {
      //判断下当前buffer下的数据是否读取完毕
      if(Read_Memory_index == Read_Memory_Size)
      {
        if(2 == W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size,BLOCK_1))
        {
          log_d("Write_ExternFlashB_After_AES_Decode read extern buffer error");
          return;
        }
        Read_Memory_index = 0;
      }
      //拷贝16个数据
      memcpy(Temp,Mem_Read_buffer + Read_Memory_index,16);
      Read_Memory_index += 16;
      //解析16个数据
      Aes_IV_key256bit_Decode(IV_IN_OUT,Temp,key256bit);//解析
      
      //写入16个byte 到外部flash 的B区 
      W25Q64_WriteData(Temp,16,BLOCK_2);
    }
		log_d("Read AppSize:%d Byte",(readTime+1)*16);
    W25Q64_WriteData_End(BLOCK_2);
    log_d("Write_ExternFlashB_After_AES_Decode end");
}

int32_t Write_Flash_From_ExternFlashB(void)
{
    u16 writeTime=0;//,flashWriteCount=0;  //写入升级包的次数 （每次写入1024个字节）	
//    u32 AppSize=0;  //升级包的大小
    u32 FlashDestination=ApplicationAddress;
    u8  Mem_Read_buffer[4096];
    u16 Read_Memory_Size=0;
    u32 ret_size = 0;

    //先读一帧，用来解析头文件格式
    ret_size = Read_Block_size(BLOCK_2);
	  log_i("size:%d",ret_size);
    uint8_t flash_state = Flash_erase(FlashDestination,ret_size);
    if (1 == flash_state)
    {
        return -1;
    }
    while(1 != W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size,BLOCK_2))
    {
      RamSource = (uint32_t)Mem_Read_buffer;
      for (writeTime = 0;writeTime<(Read_Memory_Size/4);writeTime++)
      {
        Flash_Write(FlashDestination, *(uint32_t*)RamSource);
        FlashDestination += 4;
        RamSource += 4;
      }
    }
    return ret_size;
}

void Write_ExternFlashA_From_Flash(int32_t old_app_size)
{
    u32 FlashDestination=ApplicationAddress;

    //先读一帧，用来解析头文件格式
    Erase_Flash_Block(BLOCK_1);
    W25Q64_WriteData((uint8_t *)FlashDestination,old_app_size,BLOCK_1);
    W25Q64_WriteData_End(BLOCK_1);
}

void Write_Flash_From_ExternFlashA(int32_t old_app_size)
{
    u16 writeTime=0;//,flashWriteCount=0;  //写入升级包的次数 （每次写入1024个字节）	
//    u32 AppSize=0;  //升级包的大小
    u32 FlashDestination=ApplicationAddress;
    u8  Mem_Read_buffer[4096];
    u16 Read_Memory_Size=0;
    u32 ret_size = 0;

    //先读一帧，用来解析头文件格式
    ret_size = Read_Block_size(BLOCK_1);
    uint8_t flash_state = Flash_erase(FlashDestination,ret_size);
    if (1 == flash_state)
    {
        return -1;
    }
    while(1 != W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size,BLOCK_1))
    {
      RamSource = (uint32_t)Mem_Read_buffer;
      for (writeTime = 0;writeTime<(Read_Memory_Size/4);writeTime++)
      {
        Flash_Write(FlashDestination, *(uint32_t*)RamSource);
        FlashDestination += 4;
        RamSource += 4;
      }
    }
    return ret_size;
}
