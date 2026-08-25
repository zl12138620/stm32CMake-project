/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 (Medium-density) 主程序入口
  * @note    包含按键外部中断 + 软件消抖逻辑
  ******************************************************************************
  */

#include "stm32f10x.h"

// 简单的软件延时函数，用于消抖
void Delay(__IO uint32_t nCount)
{
  for(; nCount != 0; nCount--);
}

int main(void)
{
  // PA5---按钮
  // PA6---LED
  // 1. 开启 GPIOA 和 AFIO 时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
  
  // 2. 配置 PA5 (按键)：上拉输入
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入，默认高电平
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // 3. 配置 PA6 (LED)：推挽输出
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // 初始状态：先熄灭LED（假设高电平点亮，低电平熄灭）
  GPIO_ResetBits(GPIOA, GPIO_Pin_6);
 
  // 4. 将 EXTI Line5 映射到 PA5
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
  
  EXTI_InitTypeDef EXTI_InitStruct;
  EXTI_InitStruct.EXTI_Line = EXTI_Line5;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt; 
  EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling; // 下降沿触发（按下瞬间）
  EXTI_Init(&EXTI_InitStruct);
  
  // 5. 配置 NVIC
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitTypeDef NVIC_InitStruct;
  NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;      // PA5属于 EXTI9_5 通道
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
  NVIC_Init(&NVIC_InitStruct);

  while (1)
  {
    /* 主循环空转，等待中断 */
  }
}

// 中断服务函数
void EXTI9_5_IRQHandler(void)
{
    // 1. 判断是否是 EXTI_Line5 触发的中断
    if (EXTI_GetITStatus(EXTI_Line5) == SET)
    {
        // 【关键：软件消抖逻辑】
        // 进入中断说明检测到了下降沿，但可能是抖动的假信号
        // 延时约 10ms~20ms 跳过抖动的时间段
        Delay(0xFFFF); 
        
        // 延时结束后，再次读取引脚电平，确认按键是否真的处于按下状态（低电平）
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == 0) 
        {
            // 确认是真实按下，翻转 PA6 (LED) 的电平
            if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_6) == Bit_SET)
            {
                GPIO_ResetBits(GPIOA, GPIO_Pin_6); // 置低，熄灭
            }
            else
            {
                GPIO_SetBits(GPIOA, GPIO_Pin_6);   // 置高，点亮
            }
        }
        
        // 2. 清除中断挂起位
        // 注意：如果在此期间抖动又触发了多次中断，挂起位会保持为1
        // 清除操作会确保下次真实按下时能再次进入中断
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}
