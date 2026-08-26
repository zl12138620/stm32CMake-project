#include "stm32f10x.h"

// 简单的粗略延时函数，用于按键软件消抖
void Delay(uint32_t count)
{
    while(count--);
}

int main(void)
{
    // 1. 开启时钟：GPIOA, AFIO, TIM3
    // 注意：TIM3 挂载在 APB1 总线上
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 2. 声明结构体变量（放在函数开头，防止 C89 编译器报错）
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 3. 配置 PA5 (按键)：上拉输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. 配置 PA6 (LED - TIM3_CH1)：复用推挽输出
    // 【关键】使用 PWM 时，引脚必须配置为复用推挽输出，不能是普通推挽
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 5. 配置外部中断 EXTI Line5 (PA5)
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
    EXTI_InitStructure.EXTI_Line = EXTI_Line5;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 下降沿触发（按键按下瞬间）
    EXTI_Init(&EXTI_InitStructure);

    // 6. 配置 NVIC (中断优先级)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    // 7. 配置 TIM3 时基 (产生 10kHz 的 PWM 频率)
    // 系统时钟 72MHz，预分频 71，计数频率 = 72MHz / (71+1) = 1MHz
    // 自动重装载值 99，PWM频率 = 1MHz / (99+1) = 10kHz
    TIM_TimeBaseStructure.TIM_Period = 99;       
    TIM_TimeBaseStructure.TIM_Prescaler = 71;    
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // 8. 配置 TIM3 通道 1 (PA6) 为 PWM 模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // PWM模式1：CNT < CCR 时输出高电平
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;               // 初始占空比为 0 (LED熄灭)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 高电平有效
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    
    // 使能 TIM3 通道 1 的预装载寄存器
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    
    // 9. 启动 TIM3
    TIM_Cmd(TIM3, ENABLE);

    while (1)
    {
        // 主循环空闲，按键逻辑全在中断服务函数中处理
    }
}

// 外部中断 9~5 的中断服务函数
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) == SET)
    {
        // 1. 软件消抖：粗略延时一段时间，跳过按下瞬间的机械抖动
        Delay(3000); // 约几毫秒，根据实际主频调整
        
        // 2. 再次判断按键是否处于按下状态（低电平），确认不是误触发
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == 0)
        {
            // 3. 读取当前的 PWM 比较值 (CCR1)
            uint16_t current_pulse = TIM_GetCapture1(TIM3);
            
            // 4. 在两档亮度之间切换：
            // 假设两档亮度为：暗灯 (20%亮度，CCR=20) 和 亮灯 (100%亮度，CCR=100)
            if (current_pulse == 0 || current_pulse == 20)
            {
                // 如果当前是熄灭或暗灯，切换到亮灯
                TIM_SetCompare1(TIM3, 100);
            }
            else
            {
                // 如果当前是亮灯，切换到暗灯 (20%)
                TIM_SetCompare1(TIM3, 20);
            }
        }
        
        // 5. 清除中断挂起位
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}
