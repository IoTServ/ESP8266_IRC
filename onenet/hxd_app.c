#include "esp_common.h"
#include "string.h"
#include "hxd_app.h"
#include "led.h"
#include "hxd019.h"



uint32_t g_sys_tick = 0;
//当前匹配遥控器的结果
// irlib_remote_t g_current_remote=
// {
// 	0,
// 	"美的(Media)",
// 	6,
// 	5,
// 	{0x04,0x00,0x0f,0x04,0x1d}
// };
static uint8_t s_learn_device;//要学习哪个设备，由app发数据决定:arc dvd fan iptv prj stb tv
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
	s_hxd_state = HXD_STATE_IDLE;//恢复到空闲发码状态

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
	//要学习什么设备:0-arc 1-dvd 2-fan 3-iptv 4-prj 5-stb 6-tv
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
		s_sys_tick_old = g_sys_tick;// 1s 只能发送一次，避免hxd019死掉
		//在idle状态下，才允许发送红外编码
		if (s_hxd_state == HXD_STATE_IDLE)
		{
			//根据device,keystring构造出发码数据包
			uint8_t *tbl = (uint8_t*)os_zalloc(100);
			uint8_t *ircode = (uint8_t*)os_zalloc(100);
			int outlen;
			uint8_t checksum;
			int i, j;

			//获取keycode, 获取按键码成功
			if (irlib_get_keycode(device, ircode_index, keystring, ircode, &outlen))
			{
				if (device != 0)
				{
					// 非空调设备发码
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
					// 空调设备发码
					uint8_t arcdata[7];
					
					int n = ircode_index;//组号

					//取出空调的7字节数据
					//26,1,2,3,1,2,3'\0'
					memset(arcdata, 0, 7);
					get_arc_data(arcdata, keystring);
					
					// 2bytes
					tbl[0] = 0x30;
					tbl[1] = 0x01;

					// 2bytes arc_table[]组号高低字节
					tbl[2] = (n >> 8) & 0xff;
					tbl[3] = n & 0xFF;

					//空调数据(7bytes):温度，模式，风向，电源...
					tbl[4] = arcdata[0];	// 空调温度
					tbl[5] = arcdata[1];	// 风力
					tbl[6] = arcdata[2];	// 手动
					tbl[7] = arcdata[3];	// 自动
					tbl[8] = arcdata[4];	// 开关
					tbl[9] = arcdata[5];	// 键名对应数据和显示
					tbl[10] = arcdata[6];	// 模式对应数据和显示

					//填充arc_table中的一组数据
					//取出空调遥控器的固定编码
					tbl[11] = ircode[0] + 1;
					memmove(&tbl[12], &ircode[1], tbl[11] - 1);

					i = 11 + tbl[11];	// 偏移到arc_table填充数据的后一个字节
					tbl[i] = 0xFF;		// 固定0xFF
					i++;

					//校验和
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

