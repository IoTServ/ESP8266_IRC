/*******************************************************************************
--����ң����IC hxd019��������
--zrpeng
--2015-10-10
--
--note:
hxd019c��hxd019d��������

--history:

*******************************************************************************/

#include "hxd019.h"


/******************************************************************************
 * i2c
 */
void 
i2c_init(void)
{
	PIN_FUNC_SELECT(SCL_PIN_MUX, SCL_PIN_FUNC);
	//PIN_FUNC_SELECT(SDA_PIN_MUX, SDA_PIN_FUNC);
	gpio16_output_conf();
	gpio_output_set(0, 0, 1 << SCL_PIN, 0);

	SDA_H;
	SCL_H;
}

void 
i2c_start(void)
{
	SDA_H;
	SCL_H;
	NOP5us;
	SDA_L;
	NOP5us;
	SCL_L;
}

void 
i2c_stop(void)
{
	SCL_L;
	SDA_L;
	NOP5us;
	SCL_H;
	NOP5us;
	SDA_H;
	NOP5us;
}
/*
 * �ȴ��ӻ�Ӧ���ź�
 * return��1������Ӧ��ʧ��
 *         0������Ӧ��ɹ�
 */
uint8_t 
i2c_waitack(void)
{
	uint8_t err_cnt = 0;
	SDA_IN();
	SCL_H;
	NOP5us;
	while (SDA_is_H)
	{
		err_cnt++;
		if (err_cnt > 250)
		{
			i2c_stop();
			return 1;
		}
	}
	SCL_L;
	SDA_OUT();
	NOP5us;
	return 0;
}
void 
i2c_ack(void)
{
	SCL_L;
	SDA_L;
	NOP5us;
	SCL_H;
	NOP5us;
	SCL_L;
}
void 
i2c_nack(void)
{
	SCL_L;
	SDA_H;
	NOP5us;
	SCL_H;
	NOP5us;
	SCL_L;
}
/*
 * ����һ���ֽ�
 */
void 
i2c_sendbyte(uint8_t txd)
{
	uint8_t t;
	SCL_L;
	for (t = 0; t < 8; t++)
	{
		if (txd & 0x80)
		{
			SDA_H;
		}
		else
		{
			SDA_L;
		}
		txd <<= 1;
		NOP5us;
		SCL_H;
		NOP5us;
		SCL_L;
	}
	NOP5us;
}

/*
 * ��1���ֽڲ�����
 * @ack 1:����ACK��0:����nACK
 */
uint8_t 
i2c_readbyte(uint8_t ack)
{
	uint8_t i, receive = 0;
	SDA_IN();
	SCL_L;
	NOP5us;
	for (i = 0; i < 8; i++ )
	{
		SCL_H;
		NOP5us;
		receive <<= 1;
		if (SDA_is_H)
		{
			receive |= 0X01;
		}
		SCL_L;
		NOP5us;
	}
	SDA_OUT();
	if (!ack)
	{
		i2c_nack();
	}
	else
	{
		i2c_ack();
	}
	return receive;
}




/******************************************************************************
 * hxd019
 */
static uint8_t hxd_learn_data[230];		// �洢ԭʼ��ѧϰ����

// ѧϰ�ɹ�������ֱ�ӷ����������
static hxd019_learn_callback_t learn_callback;// ѧϰ�ɹ����߳�ʱ���ᴥ���ص�����
static uint16_t learn_timeout_cnt;
static os_timer_t hxd_learn_tm;
static uint8_t learn_method = 1;			// Ĭ����ѧϰ����1, ѧϰ����2ƥ������׼ȷһЩ

/*
 * hxd019���еĿ�ͷʱ��ÿһ��i2c_startǰ����Ҫ����
 */
static void 
hxd_i2c_init(void)
{
	SDA_H;
	SCL_H;
	NOP5us;
	SCL_L;
	NOP5us;				// ����һ��������
	SCL_H;
	os_delay_us(20000);	// Ȼ�����20ms
}

void 
hxd019_init(void)
{
	i2c_init();

	PIN_FUNC_SELECT(BUSY_PIN_MUX, BUSY_PIN_FUNC);
	gpio_output_set(0, 0, 0, 1 << BUSY_PIN);
}

/*
 * д��һ��������ݣ�����hxd019�����Ӧ����ĺ����ź�.
 *
 * @*buf  �������ݣ���ʽ����:{F_code(1 byte)+KEY_code(2 byte)+COM_CODE(4 byte)+CHECKSUM}
 *        F_code,KEY_code,COM_CODE������ļ��в�ѯ.
 * @n     buf�ĳ���
 *
 * note   ѧϰ1����������: 0x30 0x00 + 112�ֽ�����
 *        ѧϰ2����������: 0x30 0x03 + 230�ֽ�����
 */
void 
hxd019_write(uint8_t *buf, int n)
{
	int i;

	hxd_i2c_init();
	i2c_start();
	for (i = 0; i < n; i++)
	{
		i2c_sendbyte(buf[i]);
		i2c_waitack();
	}
	i2c_stop();
}

/*
 * ��ȡhxd019ѧϰ���ı�������
 * @*buf  �洢ѧϰ���ı�������
 *
 * @return  0:��ȡ�ɹ�  1:��ȡʧ��
 */
uint8_t 
hxd019_read(uint8_t *buf)
{
	uint8_t value;
	uint8_t checksum;
	int i;
	uint8_t b1, b2, b3;
	int num;

	if (learn_method == 1)
	{
		b1 = 0x30;
		b2 = 0x52;
		b3 = 0x31;
		num = 110;
	}
	else if (learn_method == 2)
	{
		b1 = 0x30;
		b2 = 0x62;
		b3 = 0x31;
		num = 230;
	}
	hxd_i2c_init();
	i2c_start();
	i2c_sendbyte(b1);			// ������ַ��д
	i2c_waitack();
	i2c_sendbyte(b2);			// ѧϰ���ݵĴ洢��ַ
	i2c_waitack();
	i2c_start();				// �л�������������i2c�ź�
	i2c_sendbyte(b3);			// ������ַ����
	i2c_waitack();
	value = i2c_readbyte(1);	// ��ȡ1�ֽ�FCS(У���)

	if (value != 0)				// ���û��ѧϰ���ݣ�value��Ϊ0
	{
		i2c_stop();
		return 1;
	}

	buf[0] = value;

	checksum = b1 + b2 + b3;	// �ѷ����ֽ�b1 b2 b3У���

	for (i = 1; i < num; i++)	// ����hxd019ѧϰ���ı�������
	{
		buf[i] = i2c_readbyte(1);
		checksum += buf[i];
	}

	value = i2c_readbyte(0);	// ��ȡFCS(У���)
	i2c_stop();

	if (learn_method == 1)
	{
		if (value != checksum)
		{
			HXD019_PRINTF("hxd019_read: checksum error\n");
			return 1;
		}
		else
		{
			HXD019_PRINTF("hxd019_read: checksum ok\n");
			return 0;
		}
	}
	else if (learn_method == 2)
	{
		HXD019_PRINTF("hxd019_read: checksum ok\n");
		return 0;
	}

	return 0;
}

LOCAL void 
hxd019_learn_tm_func(void *timer_arg)
{
	learn_timeout_cnt++;
	if (learn_timeout_cnt > 2000)	// ���ж�20s��ʱ
	{
		os_timer_disarm(&hxd_learn_tm);
		if (learn_callback)
		{
			learn_callback(0, 0, HXD019_TIMEOUT);
			learn_callback = 0;
		}
		return ;
	}

	if (HXD019_BUSY_is_H)			// busy�ű�ߣ�ѧϰ���
	{
		uint8_t status;

		// ����ѧϰ���ĺ����������
		if (hxd019_read(hxd_learn_data))
		{
			HXD019_PRINTF("hxd019_read() failed\n");
			status = HXD019_FAILED;	// ��ȡʧ��
		}
		else
		{
			status = HXD019_OK;		// ��ȡok
		}

		os_timer_disarm(&hxd_learn_tm);
		if (learn_callback)
		{
			learn_callback(hxd_learn_data,
				(learn_method==1)? 110 : 230,
				status);
			learn_callback = 0;
		}
	}
}

/*
 * ����hxd019����ѧϰ״̬,���ȴ�ѧϰ����,ѧϰ�ɹ����ѧϰ���ݴ洢��hxd019_learn_data[]��
 *
 * @method  1:ѧϰ1(110�ֽ�ѧϰ����)  2:ѧϰ2(230�ֽ�ѧϰ����)
 * @func    ѧϰ����ص�
 *
 * @return  0:ѧϰ�ɹ� 1:ѧϰ��ʱ(20s��ʱ��,busy���Զ����)
 *
 * notes   1)��������ź�LSb��ǰ
 *         2)Ҫ���������շ�ͷ5cm�ڲ���ѧϰ������
 */
void 
hxd019_learn(uint8_t method, hxd019_learn_callback_t func)
{
	int n = 0;
	uint8_t b1, b2, b3;

	if (method != 1 && method != 2)
	{
		return;
	}

	learn_method = method;
	hxd_i2c_init();
	i2c_start();
	if (method == 1)
	{
		b1 = 0x30;
		b2 = 0x10;
		b3 = 0x40;
	}
	else if (method == 2)
	{
		b1 = 0x30;
		b2 = 0x20;
		b3 = 0x50;
	}
	i2c_sendbyte(b1);		// ������ַ��д
	i2c_waitack();
	i2c_sendbyte(b2);		// Ҫд�ļĴ�����ַ
	i2c_waitack();
	i2c_sendbyte(b3);		// д��Ĵ�����ֵ
	i2c_waitack();
	i2c_stop();

	os_delay_us(10000);

	// ÿ��10ms��ѯһ��busy��
	learn_callback = func;	// ���ûص�����
	learn_timeout_cnt = 0;	// ��ʱ������0
	os_timer_disarm(&hxd_learn_tm);
	os_timer_setfn(&hxd_learn_tm, (os_timer_func_t *)hxd019_learn_tm_func, 0);
	os_timer_arm(&hxd_learn_tm, 10, 1);
}



#ifdef HXD019_TEST
/******************************************************************************
 *  hxd019����
 */
/*
 * �����ʽ: 0x30+0x00+F_code(1B)+KEY_code(2B)+COM_CODE(4B)+CHECKSUM
 */
uint8_t test_tbl[][7] =
{
	// ͶӰ�ǵļ�����Ա���
	//fmt     key         custom_code
	{0x01, 0x90, 0x6f, 0x83, 0x55, 0x00, 0x00}, /*0*/
	{0x01, 0x12, 0xed, 0xE7, 0x0A, 0x00, 0x00}, /*1*/
	{0x0B, 0xaf, 0x50, 0x0d, 0x00, 0x22, 0x88}, /*2*/

	// ���ȵļ�����Ա���
	{0x63, 0x20, 0xdf, 0x03, 0x03, 0x00, 0x00}, /*0*/
	{0x63, 0x20, 0xdf, 0x03, 0x03, 0x00, 0x00}, /*1*/
	{0x01, 0x03, 0xfc, 0x01, 0xfe, 0x00, 0x00}, /*2*/

	// �����в��Ա���
	{0x01, 0x0B, 0xF4, 0x44, 0x9B, 0x00, 0x00}, /*0*/
	{0x21, 0x26, 0x0c, 0x80, 0x10, 0x00, 0x00}, /*1*/
	{0x01, 0x43, 0xBC, 0x08, 0xE6, 0x00, 0x00}, /*2*/

	// DVD���Ա���
	{0x01, 0x51, 0xAE, 0x00, 0xFF, 0x20, 0x88}, /*0*/
	{0x01, 0x5E, 0xA1, 0x00, 0x99, 0x20, 0x88}, /*1*/
	{0x01, 0x55, 0xAA, 0x01, 0xFE, 0x20, 0x88}, /*2*/

	// ���Ӳ��Ա���
	{0x04, 0x11, 0xEE, 0xC0, 0x00, 0x20, 0x88}, /*0*/
	{0x09, 0x07, 0xF8, 0x04, 0x00, 0x20, 0x88}, /*1*/  /* ����ĵڶ�����ţ�1 */
	{0x05, 0x1A, 0xE5, 0x08, 0x08, 0x20, 0x88}, /*2*/

	// ���������(IPTV)���Ա���
	{0x01, 0x43, 0xbc, 0x11, 0xee, 0x00, 0x00}, /*0*/
	{0x01, 0x9c, 0x63, 0x11, 0xee, 0x00, 0x00}, /*1*/
	{0x01, 0x0f, 0xf0, 0x01, 0xfe, 0x00, 0x00}, /*2*/

};

/*
 * �յ��������, ����ok, ȫ�������Է��������ź�
 * 0x30+0x01+(2B)+(7B)+(1B)+(?B��arc_table)+0xFF+(1B)
 */
uint8_t test_arc_tbl[][76] =
{
	/*  0 ~ 9 */
	{0x04, 0x00, 0x0F, 0x04, 0x3D},  /* 511�Ժ�(0x02,0x00,0x00),511-0,512-1.������678=167 */
	{0x04, 0x00, 0x0F, 0x04, 0x3C},
	{0x04, 0x00, 0x0F, 0x04, 0x10},
	{0x04, 0x00, 0x0F, 0x04, 0x13},
	{0x04, 0x00, 0x0F, 0x04, 0x1B},
	{0x04, 0x00, 0x0F, 0x04, 0x34},
	{0x04, 0x00, 0x0F, 0x04, 0x1D},
	{0x04, 0x00, 0x0F, 0x04, 0x5E},
	{0x04, 0x00, 0x87, 0x04, 0x2A},
	{0x04, 0x00, 0x0F, 0x04, 0x15},
	/*  10 ~ 19 */
	{0x04, 0x00, 0x87, 0x04, 0x13},
	{0x04, 0x00, 0x0F, 0x04, 0x34},
	{0x06, 0x00, 0x87, 0x04, 0xFF, 0x06, 0x1C},
	{0x06, 0x00, 0x8E, 0x04, 0xFF, 0x06, 0x1C},
	{0x06, 0x00, 0x8F, 0x04, 0xFB, 0x06, 0x1D},
	{0x06, 0x00, 0x8F, 0x04, 0xFE, 0x06, 0x1D},
	{0x06, 0x00, 0x87, 0x04, 0xFF, 0x06, 0x1D},
	{0x06, 0x00, 0x8E, 0x04, 0xFF, 0x06, 0x1D},
	{0x04, 0x00, 0x0F, 0x04, 0x1D},
	{0x06, 0x00, 0x0F, 0x04, 0x15, 0x06, 0xA5},
	/*  20 ~ 29 */
	{0x0c, 0x00, 0x23, 0x01, 0xCB, 0x02, 0x26, 0x03, 0x01, 0x05, 0x20, 0x07, 0x80},
	{0x0a, 0x00, 0x23, 0x01, 0xCB, 0x02, 0x26, 0x03, 0x01, 0x05, 0x20},
	{0x0c, 0x00, 0x11, 0x01, 0x11, 0x02, 0x11, 0x03, 0x11, 0x04, 0x11, 0x05, 0x20},
	{0x0a, 0x00, 0x11, 0x01, 0x11, 0x02, 0x11, 0x03, 0x11, 0x04, 0x11},
	{0x0a, 0x00, 0x23, 0x01, 0xCB, 0x02, 0x26, 0x03, 0x01, 0x05, 0x23},
	{0x0e, 0x00, 0x11, 0x01, 0x11, 0x02, 0x11, 0x03, 0x11, 0x04, 0x11, 0x05, 0x20, 0x08, 0x10},
	{0x0e, 0x00, 0x11, 0x01, 0x11, 0x02, 0x11, 0x03, 0x11, 0x04, 0x11, 0x05, 0x20, 0x08, 0x10},
	{
		0x2f, 0x8D, 0x71, 0x00, 0x4D, 0x01, 0xB2, 0x02, 0xf0, 0x03, 0x0F, 0x04, 0x07,
		0x05, 0xf8, 0x8D, 0x72, 0x00, 0x4D, 0x01, 0xB2, 0x02, 0xD6, 0x03, 0x29,
		0x04, 0x07, 0x05, 0xf8, 0x06, 0x4D, 0x07, 0xB2, 0x08, 0xD6, 0x09, 0x29,
		0x0A, 0x07, 0x0B, 0xf8, 0x8D, 0x73, 0x00, 0x4D, 0x02, 0xD8, 0x8D
	},
	{
		0x19, 0x8D, 0x6F, 0x00, 0x4D, 0x01, 0xB2, 0x02, 0xD6, 0x03, 0x29, 0x04, 0x07,
		0x05, 0xF8, 0x8D, 0x73, 0x00, 0x4D, 0x02, 0xD8, 0x06, 0x4D, 0x08, 0xD8,
		0x8D
	},
	{0x08, 0x00, 0x80, 0x03, 0x23, 0x05, 0x90, 0x06, 0x12},
	/*  30 ~ 39 */
	{
		0x21, 0x8D, 0x76, 0x00, 0x3f, 0x01, 0xc0, 0x02, 0x0F, 0x8D, 0x77, 0x00, 0x3f,
		0x01, 0xc0, 0x8D, 0x6F, 0x00, 0x3F, 0x8D, 0x74, 0x00, 0x3F, 0x8D, 0x6D,
		0x00, 0x3f, 0x01, 0xc0, 0x02, 0x0F, 0x04, 0x0F, 0x8D
	},
	{0x0c, 0x00, 0x4D, 0x01, 0x75, 0x02, 0xB2, 0x03, 0x8A, 0x04, 0x0F, 0x12, 0x10},
	{0x0c, 0x00, 0x4D, 0x01, 0x75, 0x02, 0xB2, 0x03, 0x8A, 0x04, 0x0F, 0x0C, 0x10},
	{0x02, 0x2, 0x55},
	{0x04, 0x00, 0x15, 0x04, 0x10},
	{
		0x1f, 0x8D, 0x71, 0x00, 0xD1, 0x01, 0x2E, 0x02, 0xD1, 0x03, 0x2E, 0x8D, 0x72,
		0x00, 0xD0, 0x01, 0x2F, 0x02, 0xD0, 0x03, 0x2F, 0x8D, 0x73, 0x00, 0xC0,
		0x02, 0x10, 0x04, 0xC0, 0x06, 0x10, 0x8D
	},
	{
		0x13, 0x8D, 0x71, 0x00, 0xD1, 0x01, 0x2E, 0x8D, 0x72, 0x00, 0xD0, 0x01, 0x2F,
		0x8D, 0x73, 0x00, 0x0C, 0x02, 0x10, 0x8D
	},
	{
		0x0f, 0x8D, 0x6F, 0x00, 0x15, 0x01, 0xEA, 0x8D, 0x73, 0x00, 0x15, 0x01, 0xEA,
		0x06, 0x0C, 0x8D
	},
	{0x04, 0x00, 0x15, 0x01, 0xC0},
	{0x04, 0x07, 0x03, 0x09, 0xCC},
};

void 
hxd019_print_learn(void)
{
	int num;
	int i;
	HXD019_PRINTF("hxd_learn_data:\n");
	if (learn_method == 1)
	{
		num = 110;
	}
	else if (learn_method == 2)
	{
		num = 230;
	}

	for (i = 0; i < num; i++)
	{
		HXD019_PRINTF("0x%x ", hxd_learn_data[i]);
	}
	HXD019_PRINTF("\n");
}

/*
 * �ǿյ��豸�������
 * @n  ���ڼ�����Ա���
 */
void 
hxd019_noarc_write_test(int n)
{
	uint8_t checksum;
	uint8_t tbl[20];
	uint8_t i;

	tbl[0] = 0x30;
	tbl[1] = 0;
	for (i = 0; i < 7; i++)
	{
		tbl[i + 2] = test_tbl[n][i];
	}

	checksum = 0;
	for (i = 0; i < 9; i++)
	{
		checksum += tbl[i];
	}
	tbl[i] = checksum;

	hxd019_write(tbl, 10);
}

/*
 * �յ��豸�������
 */
void 
hxd019_arc_write_test(int n)
{
	uint8_t checksum;
	uint8_t tbl[100];
	uint8_t i, j;
	static uint8_t sw=0;

	// 2bytes
	tbl[0] = 0x30;
	tbl[1] = 0x01;

	// 2bytes arc_table[]��Ÿߵ��ֽ�
	tbl[2] = (n >> 8) & 0xff;
	tbl[3] = n & 0xFF;

	//�յ�����(7bytes):�¶ȣ�ģʽ�����򣬵�Դ...
	tbl[4] = 25;	// �յ��¶�
	tbl[5] = 4;		// ����
	tbl[6] = 2;		// �ֶ�
	tbl[7] = 1;		// �Զ�
	tbl[8] = !sw;// ����
	tbl[9] = 1;		// ������Ӧ���ݺ���ʾ
	tbl[10] = 1;	// ģʽ��Ӧ���ݺ���ʾ

	//���arc_table�е�һ������
	tbl[11] = test_arc_tbl[n][0] + 1;
	memmove(&tbl[12], &test_arc_tbl[n][1], tbl[11] - 1);

	i = 11 + tbl[11];	// ƫ�Ƶ�arc_table������ݵĺ�һ���ֽ�
	tbl[i] = 0xFF;		// �̶�0xFF
	i++;

	//У���
	checksum = 0;
	for (j = 0; j < i; j++)
	{
		checksum += tbl[j];
	}
	tbl[i] = checksum;
	i++;

	hxd019_write(tbl, i);
}

void 
learn_test_func(uint8_t *data, int length, uint8 status)
{
	if (status == HXD019_OK)
	{
		HXD019_PRINTF("learn_test_func: learn ok\n");
		hxd019_print_learn();
	}
	else if (status == HXD019_TIMEOUT)
	{
		HXD019_PRINTF("learn_test_func: learn timeout\n");
	}
}

void 
hxd019_learn_test(uint8_t method)
{
	hxd019_learn(method, learn_test_func);
}

void 
hxd019_learn_write_test(uint8_t *buf, int n)
{
	int i;

	hxd_i2c_init();

	i2c_start();
	i2c_sendbyte(0x30);		// ������ַ��д
	i2c_waitack();
	i2c_sendbyte(0x00);		// Ҫд�ļĴ�����ַ
	i2c_waitack();

	for (i = 0; i < n; i++)
	{
		i2c_sendbyte(buf[i]);
		i2c_waitack();
	}
	i2c_stop();
}
#endif

