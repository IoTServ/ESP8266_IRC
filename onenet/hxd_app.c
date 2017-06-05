#include "esp_common.h"
#include "string.h"
#include "hxd_app.h"
#include "led.h"
#include "hxd019.h"



uint32_t g_sys_tick = 0;
//��ǰƥ��ң�����Ľ��
// irlib_remote_t g_current_remote=
// {
// 	0,
// 	"����(Media)",
// 	6,
// 	5,
// 	{0x04,0x00,0x0f,0x04,0x1d}
// };
static uint8_t s_learn_device;//Ҫѧϰ�ĸ��豸����app�����ݾ���:arc dvd fan iptv prj stb tv
static uint8_t s_hxd_state;
static char s_hxd_state_string[100];
static uint32_t s_sys_tick_old;

static void  learn_cb(uint8_t *data, int length, uint8_t state)
{
	if (state == HXD019_OK)
	{
		DPRINTF("learn ok\n");
		// irlib_match_remote(s_learn_device, data, length, &g_current_remote);
		
		//os_sprintf(s_hxd_state_string, "%s,study_ok", brand[s_learn_device]);
	}
	else if (state == HXD019_TIMEOUT)
	{
		DPRINTF("learn timeout\n");

		//os_sprintf(s_hxd_state_string, "%s,study_failed", brand[s_learn_device]);
	}
	else if (state == HXD019_FAILED)
	{
		//os_sprintf(s_hxd_state_string, "%s,study_failed", brand[s_learn_device]);
	}
	s_hxd_state = HXD_STATE_IDLE;//�ָ������з���״̬

	char *buf;
	buf = (uint8_t*)os_zalloc(100);
	os_sprintf(buf, "rpt:hxd,status,%s#", s_hxd_state_string);
	gizwits_app_send(buf);
	os_free(buf);
}

const char *hxd_get_status_string(void)
{
	return s_hxd_state_string;
}

 void get_arc_data(uint8_t *arcdata, uint8_t *keystring)
{
	int i;
	char *pch = keystring;

	DPRINTF("%s\n", keystring);
	for (i=0; i<7; i++)
	{
		arcdata[i] = _atoi(pch);
		pch = strchr(pch, ',');
		if (pch == NULL)
			break;
		pch++;
	}
	DPRINTF("\n");
}

uint8_t hxd_get_state(void)
{
	return s_hxd_state;
}

void  hxd_learn(uint8_t dev, uint8_t method)
{
	//Ҫѧϰʲô�豸:0-arc 1-dvd 2-fan 3-iptv 4-prj 5-stb 6-tv
	s_learn_device = dev;
	if (s_hxd_state == HXD_STATE_IDLE)
	{
		s_hxd_state = HXD_STATE_LEARN;
		hxd019_learn(method, learn_cb);
		DPRINTF("HXD_LEARN_EVT, %d %d\n", dev, method);
	}
}

void  hxd_send(uint8_t device, uint8_t *keystring, uint16_t ircode_index)
{
	DPRINTF("HXD_SEND_EVT, %d %s %d\n", device, keystring, ircode_index);

	if (s_sys_tick_old != g_sys_tick)
	{
		s_sys_tick_old = g_sys_tick;// 1s ֻ�ܷ���һ�Σ�����hxd019����
		//��idle״̬�£��������ͺ������
		if (s_hxd_state == HXD_STATE_IDLE)
		{
			//����device,keystring������������ݰ�
			uint8_t *tbl = (uint8_t*)os_zalloc(100);
			uint8_t *ircode = (uint8_t*)os_zalloc(100);
			int outlen;
			uint8_t checksum;
			int i, j;

			//��ȡkeycode, ��ȡ������ɹ�
			if (irlib_get_keycode(device, ircode_index, keystring, ircode, &outlen))
			{
				if (device != 0)
				{
					// �ǿյ��豸����
					tbl[0] = 0x30;
					tbl[1] = 0;
					for (i = 0; i < outlen; i++)
					{
						tbl[i + 2] = ircode[i];
					}

					checksum = 0;
					for (i = 0; i < 9; i++)
					{
						checksum += tbl[i];
					}
					tbl[i] = checksum;

					hxd019_write(tbl, 10);

					DPRINTF("nonarc tbl:");
					for (i=0; i<10; i++)
					{
						DPRINTF("%02x ", tbl[i]);
					}
					DPRINTF("\n");
				}
				else
				{
					// �յ��豸����
					uint8_t arcdata[7];
					
					int n = ircode_index;//���

					//ȡ���յ���7�ֽ�����
					//26,1,2,3,1,2,3'\0'
					memset(arcdata, 0, 7);
					get_arc_data(arcdata, keystring);
					
					// 2bytes
					tbl[0] = 0x30;
					tbl[1] = 0x01;

					// 2bytes arc_table[]��Ÿߵ��ֽ�
					tbl[2] = (n >> 8) & 0xff;
					tbl[3] = n & 0xFF;

					//�յ�����(7bytes):�¶ȣ�ģʽ�����򣬵�Դ...
					tbl[4] = arcdata[0];	// �յ��¶�
					tbl[5] = arcdata[1];	// ����
					tbl[6] = arcdata[2];	// �ֶ�
					tbl[7] = arcdata[3];	// �Զ�
					tbl[8] = arcdata[4];	// ����
					tbl[9] = arcdata[5];	// ������Ӧ���ݺ���ʾ
					tbl[10] = arcdata[6];	// ģʽ��Ӧ���ݺ���ʾ

					//���arc_table�е�һ������
					//ȡ���յ�ң�����Ĺ̶�����
					tbl[11] = ircode[0] + 1;
					memmove(&tbl[12], &ircode[1], tbl[11] - 1);

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

					DPRINTF("arc tbl:");
					for (j=0; j<i; j++)
					{
						DPRINTF("%02x ", tbl[j]);
					}
					DPRINTF("\n");
				}
			}
			else
			{
				DPRINTF("get keycode error\n");
			}
			os_free(tbl);
			os_free(ircode);
		}
	}
}

