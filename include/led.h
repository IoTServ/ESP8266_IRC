#ifndef _LED_H_
#define _LED_H_

#include "esp_common.h"
#include "gpio.h"

/*****************************************************************************
 * led pin definition
 */
#define RED_PIN_MUX		PERIPHS_IO_MUX_MTCK_U
#define RED_PIN_FUNC	FUNC_GPIO13
#define RED_PIN			13

#define BLUE_PIN_MUX	PERIPHS_IO_MUX_MTDO_U
#define BLUE_PIN_FUNC	FUNC_GPIO15
#define BLUE_PIN		15

#define	RED_L			GPIO_OUTPUT_SET(RED_PIN,0)
#define	RED_H			GPIO_OUTPUT_SET(RED_PIN,1)

#define	BLUE_L			GPIO_OUTPUT_SET(BLUE_PIN,0)
#define	BLUE_H			GPIO_OUTPUT_SET(BLUE_PIN,1)

enum
{
	RED = 0,
	BLUE,
};

enum
{
	LED_OFF = 0,
	LED_ON,
};
/*
 * public function
 */
void led_init(void);
void led_set(uint8_t led, uint8_t mode);

#endif


