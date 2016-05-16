#include "platform.h"
#include "drivers/light_ws2811strip_configs.h"

const ws2811_t *ws2811_current;

void initLEDMapping()
{
#ifdef NAZE
	ws2811_current = &ws2811hw[lrm_System];
#endif
}

#ifdef NAZE
PG_REGISTER_WITH_RESET_FN(uint8_t, lrm, PG_LED_REMAP_CONFIG, 0);   // ws2811hw index

void pgResetFn_lrm(uint8_t *instance)
{
	*instance = 0;
}

#endif

#if defined(NAZE)
const ws2811_t ws2811hw[LED_STRIP_REMAP_OPTS] = {
    // Initial NAZE LED_STRIP config, conflicting with SOFTSERIAL
    {
        GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_6,
        TIM3,  RCC_APB1Periph_TIM3,  TIM_CH1, 
        DMA1_Channel6, DMA1_FLAG_TC6, DMA1_CH6_HANDLER, DMA1_Channel6_IRQn
    },
    // Alternate NAZE LED_STRIP config, conflicting with RSSI_ADC
    {
        GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_1,
        TIM2,  RCC_APB1Periph_TIM2,  TIM_CH2, 
        DMA1_Channel7, DMA1_FLAG_TC7, DMA1_CH7_HANDLER, DMA1_Channel7_IRQn
    },
};
#elif defined(CC3D)
const ws2811_t ws2811hw[LED_STRIP_REMAP_OPTS] = {
    // CC3D LED strip configuration is a bit different
    {
        GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_4,
        TIM3,  RCC_APB1Periph_TIM3,  TIM_CH1, 
        DMA1_Channel6, DMA1_FLAG_TC6, DMA1_CH6_HANDLER, DMA1_Channel6_IRQn
    }
};
#else
const ws2811_t ws2811hw[LED_STRIP_REMAP_OPTS] = {
    // any other FC configuration
    {
        GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_6,
        TIM3,  RCC_APB1Periph_TIM3,  TIM_CH1, 
        DMA1_Channel6, DMA1_FLAG_TC6, DMA1_CH6_HANDLER, DMA1_Channel6_IRQn
    },
};
#endif
