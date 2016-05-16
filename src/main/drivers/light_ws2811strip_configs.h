//#include "drivers/light_ws2811strip.h"

#ifdef NAZE
#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"
#include "drivers/dma.h"

PG_DECLARE(uint8_t, lrm);
#endif

typedef struct {
	GPIO_TypeDef *gpio;		// GPIOx
	uint32_t gpio_rcc;		// RCC_APB2Periph_GPIOx
	uint16_t gpio_pin;		// GPIO_Pin_x
	TIM_TypeDef *tim;		// TIMx
	uint32_t tim_rcc;		// RCC_APB1Periph_TIMx
	uint8_t tim_channel;	// TIMx channel index (TIM_OCx(Init/TIM_OC1PreloadConfig))
	//DMA_TypeDef *dma;	// It's always DMA1 for now
	DMA_Channel_TypeDef *dma_channel;	// DMA1_Channelx
	uint32_t dma_tc_flag;	// DMA1_FLAG_TCx
	uint8_t dma_interrupt_handler; // DMA1_CHx_HANDLER, from drivers/dma.h
	uint8_t dma_channel_irq;	// DMA1_Channelx_IRQn
} ws2811_t;

typedef enum {TIM_CH1 = 0, TIM_CH2, TIM_CH3, TIM_CH4} timerChannel_t;

#if defined(NAZE)
#define LED_STRIP_REMAP_OPTS	2
#else
#define LED_STRIP_REMAP_OPTS	1
#endif

extern const ws2811_t ws2811hw[LED_STRIP_REMAP_OPTS];
extern const ws2811_t *ws2811_current;

void initLEDMapping();
