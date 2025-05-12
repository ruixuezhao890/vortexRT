//
// Created by ruixuezhao on 25-5-11.
//

#ifndef SYSTICK_HPP
#define SYSTICK_HPP

//更换芯片头文件也更换
#include "stm32f4xx.h"
#include "core_cm4.h"
#include "stm32f4xx_hal.h"

#ifndef HSE_VALUE
#define HSE_VALUE 8000000U  // 8MHz 外部晶振
#endif

//一般不定义用hal库默认，需要时取消下面注释修改
//#ifndef HSI_VALUE
//#define HSI_VALUE 8000000U  // 8MHz 内部 RC
//#endif

struct SystemClockConfig {
    uint32_t sysclk;   // 系统时钟 (SYSCLK)
    uint32_t hclk;     // AHB 总线时钟 (HCLK)
    uint32_t pclk1;    // APB1 总线时钟 (PCLK1)
    uint32_t pclk2;    // APB2 总线时钟 (PCLK2)
    uint32_t timclk1;  // APB1 定时器时钟 (TIM2-TIM4)
    uint32_t timclk2;  // APB2 定时器时钟 (TIM1, TIM8)
    uint32_t adcclk;   // ADC 时钟
};
#endif //SYSTICK_HPP
