/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 (Medium-density) 主程序入口
  * @note    这是占位程序，请按需替换。
  *          SystemInit() 已由启动文件 Reset_Handler 在进入 main 之前自动调用。
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "stm32f10x_conf.h"

int main(void)
{
  /* TODO: 在此编写用户应用代码 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  
  while (1)
  {
    
    
  }
}
