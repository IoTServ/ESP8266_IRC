#ifndef _HXD_APP_H_
#define _HXD_APP_H_


enum
{
	HXD_STATE_IDLE,
	HXD_STATE_LEARN,
};




void  hxd_learn(uint8_t dev, uint8_t method);
void  hxd_send(uint8_t device, uint8_t *keystring, uint16_t ircode_index);
uint8_t hxd_get_state(void);

#endif
