#ifndef __BSP_LED_H
#define __BSP_LED_H
#include "sys.h"


//LED�˿ڶ���
#define RCC_ALL_LED     (RCC_AHB1Periph_GPIOG)

#define GPIO_PORT          GPIOG

#define GPIO_PIN_LED1       GPIO_Pin_12
#define GPIO_PIN_ERRORLED   GPIO_Pin_8

#define GPIO_PIN_SWITCH     GPIO_Pin_3





//LED�˿ڶ���
#define LEDERROR PGout(8)	
#define LED1 PGout(12)	
#define SWITCH_ON_OFF PGout(3)	



void bsp_LED_Init(void);//��ʼ��		 	



//void bsp_LedToggle(uint8_t _no);
#endif
