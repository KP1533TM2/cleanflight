/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>

#include <platform.h>

#include "common/color.h"
#include "drivers/light_ws2811strip.h"
#include "drivers/light_ws2811strip_configs.h"
#include "nvic.h"

const uint16_t tim_dma_ccs[4] = {TIM_DMA_CC1, TIM_DMA_CC2, TIM_DMA_CC3, TIM_DMA_CC4};

/* Workarounds around StdPeriphLib idiosyncrasies */
void (* const TIM_OCInit[4]) (TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct) = 
    {TIM_OC1Init, TIM_OC2Init, TIM_OC3Init, TIM_OC4Init};
void (* const TIM_OCPreloadConfig[4]) (TIM_TypeDef* TIMx, uint16_t TIM_OCPreload) = 
    {TIM_OC1PreloadConfig, TIM_OC2PreloadConfig, TIM_OC3PreloadConfig, TIM_OC4PreloadConfig};

void ws2811LedStripHardwareInit(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    uint16_t prescalerValue;
   
    RCC_APB2PeriphClockCmd(ws2811_current->gpio_rcc, ENABLE);

    /* GPIOA Configuration: TIM3 Channel 1 as alternate function push-pull */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ws2811_current->gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    RCC_APB1PeriphClockCmd(ws2811_current->tim_rcc, ENABLE);
    /* Compute the prescaler value */
    prescalerValue = (uint16_t) (SystemCoreClock / 24000000) - 1;
    /* Time base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 29; // 800kHz
    TIM_TimeBaseStructure.TIM_Prescaler = prescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(ws2811_current->tim, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel1 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    
    /* XXX Notice how those aren't StdPeriphLib functions, but rather
     * arrays of pointers to them.
     */
    TIM_OCInit[ws2811_current->tim_channel](ws2811_current->tim, &TIM_OCInitStructure);
    TIM_OCPreloadConfig[ws2811_current->tim_channel](ws2811_current->tim, TIM_OCPreload_Enable);

    TIM_CtrlPWMOutputs(ws2811_current->tim, ENABLE);

    /* configure DMA */
    /* DMA clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* DMA1 Channel6 Config */
    DMA_DeInit(ws2811_current->dma_channel);

    DMA_StructInit(&DMA_InitStructure);
    uint32_t pba;
    switch(ws2811_current->tim_channel)
    {
    	case TIM_CH1: pba = (uint32_t)&ws2811_current->tim->CCR1; break;
    	case TIM_CH2: pba = (uint32_t)&ws2811_current->tim->CCR2; break;
    	case TIM_CH3: pba = (uint32_t)&ws2811_current->tim->CCR3; break;
    	case TIM_CH4: pba = (uint32_t)&ws2811_current->tim->CCR4; break;
    }
    DMA_InitStructure.DMA_PeripheralBaseAddr = pba;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ledStripDMABuffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = WS2811_DMA_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(ws2811_current->dma_channel, &DMA_InitStructure);

    /* TIM3 CC1 DMA Request enable */
    TIM_DMACmd(ws2811_current->tim, tim_dma_ccs[ws2811_current->tim_channel], ENABLE);

    DMA_ITConfig(ws2811_current->dma_channel, DMA_IT_TC, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = ws2811_current->dma_channel_irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = NVIC_PRIORITY_BASE(NVIC_PRIO_WS2811_DMA);
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = NVIC_PRIORITY_SUB(NVIC_PRIO_WS2811_DMA);
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    setStripColor(&hsv_white);
    ws2811UpdateStrip();
}

void ws2811LedStripDMAEnable(void)
{
    DMA_SetCurrDataCounter(ws2811_current->dma_channel, WS2811_DMA_BUFFER_SIZE);  // load number of bytes to be transferred
    TIM_SetCounter(ws2811_current->tim, 0);
    TIM_Cmd(ws2811_current->tim, ENABLE);
    DMA_Cmd(ws2811_current->dma_channel, ENABLE);
}

