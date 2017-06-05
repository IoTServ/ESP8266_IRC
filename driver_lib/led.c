#include "led.h"


void led_init(void)
{
	PIN_FUNC_SELECT(RED_PIN_MUX, RED_PIN_FUNC);
	PIN_FUNC_SELECT(BLUE_PIN_MUX, BLUE_PIN_FUNC);

	RED_L;
	BLUE_L;
}

void led_set(uint8_t led, uint8_t mode)
{
	if (led == RED)
	{
		switch (mode)
		{
		case LED_OFF:
			RED_L;
			break;

		case LED_ON:
			RED_H;
			break;
		}
	}
	else if (led == BLUE)
	{
		switch (mode)
		{
		case LED_OFF:
			BLUE_L;
			break;

		case LED_ON:
			BLUE_H;
			break;
		}
	}
}
