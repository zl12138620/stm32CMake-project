/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 (Medium-density) 主程序入口
  * @note    这是占位程序，请按需替换。
  *          SystemInit() 已由启动文件 Reset_Handler 在进入 main 之前自动调用。
  ******************************************************************************
  */

#include "stm32f10x.h"


int main(void)
{
  /* TODO: 在此编写用户应用代码 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
  GPIO_InitTypeDef gpio_test;
  gpio_test.GPIO_Mode = GPIO_Mode_Out_PP;
  gpio_test.GPIO_Pin = GPIO_Pin_5;
  gpio_test.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &gpio_test);

  
  //配置AFIO时钟，用于启动中断
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
  EXTI_InitTypeDef EXTI_InitStruct;
  EXTI_InitStruct.EXTI_Line = EXTI_Line5;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt; //事件中断
  EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_Init(&EXTI_InitStruct);
  
  //优先级分组
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitTypeDef NVIC_InitStruct;
  NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
  NVIC_Init(&NVIC_InitStruct);


  
  // RCC->APB2ENR = 0x00000004;
  // GPIOA->CRL = 0x00200000;
  // GPIOA->ODR = 0x00000020;
  while (1)
  {
    /* code */
  }

}
