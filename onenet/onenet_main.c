/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		onenet_main.c
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/02/06
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/02/06		宋伟		  1.0.0			file created
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "esp_common.h"

#include "onenet/onenet_rgmp.h"

#include "onenet_err.h"
#include "network.h"
#include "params.h"
#include "tlv.h"
#include "device_common.h"
#include "driver_lib/uart.h"

#include "onenet_main.h"
#include "led.h"
#include "hxd019.h"
#include "hxd_app.h"
#include "key.h"
/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define MODULE_VER_MAJOR "1"
#define MODULE_VER_MINOR "0"

#define TASK_DELAY_INTERVAL_IN_MS 1000
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct module_time
{
	uint8 valid;
	uint8 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minite;
	uint8 second;
};

#define STM_ENTRY_SIG 0x0001
#define DRIVER_LED_EVT 0x0002
#define SYS_TICK_EVT 0x0004
/**
 *******************************************************************************
 * @brief      gpio config
 *******************************************************************************
 */
#define Reset_Pin_NUM         4
#define Reset_Pin_FUNC        FUNC_GPIO4
#define Reset_Pin_MUX         PERIPHS_IO_MUX_GPIO4_U


/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//static os_timer_t send_request_timer;
static struct device *device_ctx = NULL;
static uint8 cloud_link_running = 0;
static uint8 onenet_main_running = 0;

LOCAL os_timer_t dispatch_timer;
LOCAL os_timer_t led_timer;
LOCAL os_timer_t key_timer;

xQueueHandle xQueueDevice;
LOCAL uint16_t device_event;

LOCAL mcu_status_t s_mcu_status;
LOCAL system_status_t s_system_status;
static os_timer_t s_manufacture_timer;
int s_manufacture_timeout = 0;
static int s_wifi_is_scanning = 0;
static int s_manufacture_mode = 0;
static int s_upgrade_request = 0;


static uint8_t s_blink_state=0;
static uint32_t s_blink_cnt=0;
static uint8_t s_blink_cnt_toggle;
static uint8_t s_blink_cnt_repeat;

    /**
 *******************************************************************************
 * @brief     ������ر���?
 *******************************************************************************
 */
static struct keys_param switch_param;
static struct single_key_param *switch_signle;
static bool status = true;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
extern uint32_t g_sys_tick;
/*****************************************************************************/
/* External Functions and Variables                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void Switch_LongPress_Handler( void );
static void drv_Switch_Init( void );
/*
 * 接收数据
 */
void onenet_cloud_receive(const unsigned char *pdata, unsigned short length)
{
	// app->mcu:
	//   获取设�?�属性�?, [0]:0x02-�??
	//   设置设�?�属性�?  [0]:0x01-�??  [1]:属性标记位  [2-x]:属性�?
	//
	// note:length的大小是packed之后的属性值结构体字节大小
	if (length == 1 && pdata[0] == 0x02)
	{
		mcu_status_t *mcu_status;

		// !!! 报告数据给app, [0]:0x03  [1-x]:属性�?
		uint8 mcu_data[IRCODE_LEN + 1];
		mcu_data[0] = 0x03;
		mcu_status = (mcu_status_t *)(&mcu_data[1]);
		memcpy(mcu_status->ircode, s_mcu_status.ircode_save, IRCODE_LEN);
		onenet_cloud_send(mcu_data, sizeof(mcu_data) / sizeof(uint8));
	}
	else if (length == (IRCODE_LEN + 2) && pdata[0] == 0x01)
	{
		mcu_attr_flags_t *mcu_attr_flags = (mcu_attr_flags_t *)(&pdata[1]);
		mcu_status_t *mcu_status;

		mcu_status = (mcu_status_t *)(&pdata[2]);

		if (mcu_attr_flags->setting_ircode)
		{
			memcpy(s_mcu_status.ircode, mcu_status->ircode, IRCODE_LEN);
			int i;
			os_printf("setting_ircode:");
			for (i = 0; i < IRCODE_LEN; i++)
			{
				os_printf("%c", mcu_status->ircode[i]);
			}
			os_printf("\r\n");
		}
		if (os_strstr(s_mcu_status.ircode, "ircode") != 0)
		{
			os_memcpy(s_mcu_status.ircode_save, s_mcu_status.ircode, IRCODE_LEN);
		}

		// s_mcu_status -->  mcu driver
		onenet_app_apply_settings(); //根据数据点状态来控制�??�??

		// s_mcu_status  -->  flash
		onenet_app_save(); // 保存当前的数�??点状态信�??
	}
}

void onenet_app_apply_settings(void)
{
	int length;
	char *pch = strchr(s_mcu_status.ircode, '#');
	if (pch)
	{
		pch++;
		*pch = 0;
		length = strlen(s_mcu_status.ircode);

		cld_recv_data(s_mcu_status.ircode, length);
	}
}

void onenet_app_send(char *s)
{
	uint8 mcu_data[IRCODE_LEN + 1];
	mcu_status_t *mcu_status;

	memset(mcu_data, 0, sizeof(mcu_data) / sizeof(uint8));
	mcu_data[0] = 0x03;
	mcu_status = (mcu_status_t *)(&mcu_data[1]);
	os_sprintf(mcu_status->ircode, "%s", s);
	onenet_cloud_send(mcu_data, sizeof(mcu_data) / sizeof(uint8));
	//onenet_lan_tcp_server_dev2app(mcu_data, sizeof(mcu_data) / sizeof(uint8));
}

/*
 * 加载local_mcu_status参数
 */
void onenet_app_load(void)
{
	spi_flash_read((ONENET_APP_START_SEC), (uint32_t *)(&s_system_status), sizeof(s_system_status));

	s_mcu_status = s_system_status.mcu_status;
	if (s_system_status.init_flag != 1)
	{
		//os_strncpy(s_mcu_status.ircode_save, "hxd:arc,ircode,830,26,3,2,0,1,0,2#", IRCODE_LEN);
		s_system_status.init_flag = 1;
	}
	s_system_status.start_count += 1;
	s_system_status.start_continue += 1;
	os_printf("Onenet APP: start count:%d,start_continue:%d\r\n", s_system_status.start_count, s_system_status.start_continue);
	onenet_app_save();
}

void onenet_app_save(void)
{
	s_system_status.mcu_status = s_mcu_status; //结构体拷�??

	spi_flash_erase_sector(ONENET_APP_START);

	spi_flash_write(
		(ONENET_APP_START_SEC),
		(uint32_t *)(&s_system_status),
		sizeof(s_system_status));
}

static void manufacture_check(void *timer_arg)
{
	s_manufacture_timeout++;
	if (s_manufacture_timeout > 60 * 60 * 24) //24H
	{
		system_restart();
		return;
	}
	hxd019_arc_write_test(0);
}

static void scan_done(void *arg, STATUS status)
{
	uint8 ssid[33];
	char temp[128];

	if (status == OK)
	{
		struct bss_info *bss_link = (struct bss_info *)arg;
		bss_link = bss_link->next.stqe_next; //ignore the first one , it's invalid.

		while (bss_link != NULL)
		{
			memset(ssid, 0, 33);
			if (strlen(bss_link->ssid) <= 32)
			{
				memcpy(ssid, bss_link->ssid, strlen(bss_link->ssid));
			}
			else
			{
				memcpy(ssid, bss_link->ssid, 32);
			}
			os_printf("(%d,\"%s\",%d,\"" MACSTR "\",%d)\r\n",
					  bss_link->authmode, ssid, bss_link->rssi,
					  MAC2STR(bss_link->bssid), bss_link->channel);
			// 如果产测�??由器在附近则进入产测模式
			if (strcmp("aibox-manufacture-test", ssid) == 0)
			{
				// 开始产�??, 24hour
				s_manufacture_timeout = 1;
				os_timer_disarm(&s_manufacture_timer);
				os_timer_setfn(&s_manufacture_timer, manufacture_check, 0);
				os_timer_arm(&s_manufacture_timer, 1000, 1);

				// 连接hekr-test
				struct station_config stationConf;
				wifi_station_get_config(&stationConf);

				strcpy((char*)&stationConf.ssid, ssid);

				strcpy((char*)&stationConf.password, "aibox0.123456789");
				wifi_station_set_config(&stationConf);
				wifi_station_disconnect();
				wifi_station_connect();
				break;
			}
			bss_link = bss_link->next.stqe_next;
		}
	}
	else
	{
		os_printf("scan fail !!!\r\n");
	}
	s_wifi_is_scanning = 0;
}

static void wifi_scan_manufacture_ssid(void)
{
	if (wifi_get_opmode() == SOFTAP_MODE)
	{
		os_printf("ap mode can't scan !!!\r\n");
		return;
	}
	wifi_station_scan(NULL, scan_done);
}

void onenet_app_tick(uint32_t tick)
{
	if (tick == 10)
	{
		s_system_status.start_continue = 0;
		onenet_app_save();
	}

	// 5次进入配网模�?
	if (s_system_status.start_continue >= 5)
	{
		s_system_status.start_continue = 0;
		os_printf("network_start_smartconfig\r\n");
		onenet_app_save();
		network_start_smartconfig(SMARTCONFIG_TYPE_ESPTOUCH);
	}
	 else if (s_system_status.start_continue == 4)
	 {
	 	s_system_status.start_continue = 0;
		onenet_app_save();
	 }
	else if (s_system_status.start_continue == 3)
	{
		s_manufacture_mode = 1;
		os_printf("s_manufacture_mode\r\n");
		network_start_smartconfig(SMARTCONFIG_TYPE_ESPTOUCH);
		os_printf("s_manufacture_mode11\r\n");
	}

	//产测模式: 1s发射一次红�??
	if (s_manufacture_mode == 1)
	{
		if (s_manufacture_timeout == 0)
		{
			if (s_wifi_is_scanning == 0)
			{
				s_wifi_is_scanning = 1;
				wifi_scan_manufacture_ssid();
			}
		}
	}

	// 60s升级
	// if ((s_upgrade_request == 1) && (tick < 60))
	// {
	// 	//联网状态下才会处理固件升级事件
	// 	os_post_message(NETWORK_ID, NW_UPGRADE_EVT, 0);
	// 	if (update_get_state() == 1)
	// 	{
	// 		s_upgrade_request = 0;
	// 	}
	// }
}

static void driver_actor(void *arg)
{
	uint16_t *event = (uint16_t *)arg;

	device_event |= *event;

	xQueueSend(xQueueDevice, (void *)&device_event, 0);
}

static void netlink_callback(enum network_status status)
{
	os_printf("%s data changed %d -> %d\n", __FUNCTION__, device_ctx->module_status.net_link_flag, status);
	if (status != device_ctx->module_status.net_link_flag)
	{
		if ((ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status) && (NETWORK_LINKING == status))
			device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_LINK_FAILED;
	}
	device_ctx->module_status.net_link_flag = status;
}

static int onenet_device_send_data(uint8 *data, uint16 data_len)
{
	int32 ret = ONENET_SUCCESS;

	if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status)
	{
		if (ONENET_UPLOAD_STATUS_IDLE == device_ctx->upload_status)
		{
			device_ctx->data->sensor_data[0].data = data;
			device_ctx->data->sensor_data[0].len = data_len;
			device_ctx->data->sensor_data[0].timestamp = 0;
			device_ctx->data->sd_count = 1;
			os_printf("Send Data ====\n");
			device_ctx->upload_status =
				ONENET_UPLOAD_STATUS_WAIT_TO_SEND;
		}
		else
			ret = ONENET_RESOURCE_BUSY;
	}
	else
	{
		/** 返回错�??*/
		ret = ONENET_DEVICE_OFFLINE;
	}

	return ret;
}

static int onenet_device_send_event(uint8 *data, uint16 data_len)
{
	int32 ret = ONENET_SUCCESS;

	if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status)
	{
		if (ONENET_UPLOAD_STATUS_IDLE == device_ctx->upload_status)
		{
			device_ctx->data->event.event = data + 1;
			device_ctx->data->event.len = data_len - 1;
			device_ctx->data->event.timestamp = 0;
			device_ctx->data->event.level = *data;
			device_ctx->upload_status =
				ONENET_UPLOAD_STATUS_WAIT_TO_SEND;
		}
		else
			ret = ONENET_RESOURCE_BUSY;
	}
	else
	{
		/** 返回错�??*/
		ret = ONENET_DEVICE_OFFLINE;
	}

	return ret;
}

static void set_module_info(uint8 *register_code, uint8 *product_uuid,
							uint8 *product_ident, uint8 *product_ver, uint8 *profile,
							uint32 profile_len)
{
	strcpy(device_ctx->register_code, register_code);
	strcpy(device_ctx->product_uuid, product_uuid);
	strcpy(device_ctx->product_ident, product_ident);
	strcpy(device_ctx->product_ver, product_ver);
	if (device_ctx->profile)
		os_free(device_ctx->profile);
	device_ctx->profile = os_malloc(profile_len);
	memcpy(device_ctx->profile, profile, profile_len);
	device_ctx->profile_len = profile_len;
}

static void onenet_device_bind(void)
{
	uint8 *dev_name = os_malloc(64);
	memset(dev_name, 0, 64);
	sprintf(dev_name, "%s_%s", device_ctx->product_ident, params_get_unique_id());
	device_bind_stop();
	device_bind_start(device_ctx->dev_id, device_ctx->acc_token,
					  dev_name, device_ctx->product_ident);
	os_printf("Start device bind \n");
	os_free(dev_name);
}

static void onenet_device_notification_callback(void *context,
												const rgmp_notification_id_t *id, const char *notification, size_t size)
{
	/** 处理下发的通知，格式为tlv*/
}

static void onenet_device_software_update_callback(void *context,
												   const rgmp_module_update_info_t *modules, int count)
{
	/** 仅�?�录固件包信�???，由其它任务来执行升�???*/
}

static int onenet_device_configuration_callback(void *context,
												const char *configuration, size_t size)
{
	/** 处理下发的配�???，格式为tlv*/
}

static void cloud_link_task(void *params)
{
	int32 err = 0;

	cloud_link_running = 1;

	while (cloud_link_running)
	{
		/** 初�?�状态时，创建�?��??*/
		if (ONENET_DEVICE_STATUS_START_LINK == device_ctx->cloud_link_status)
		{
			os_printf("Create Device\n");
			err = onenet_device_create(device_ctx->product_uuid,
									   device_ctx->register_code, &(device_ctx->dev));
			if (ONENET_SUCCESS == err)
			{
				os_printf("Create Device success\n");
				device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_CREATED;
			}
		}

		/** 设�?�创建成功后，或设�?�掉线时，重新打开设�?�进行网络重�???*/
		if ((ONENET_DEVICE_STATUS_CREATED == device_ctx->cloud_link_status) || (ONENET_DEVICE_STATUS_LINK_FAILED == device_ctx->cloud_link_status))
		{
			os_printf("Open Device \n");
			err = onenet_device_open(device_ctx->dev, device_ctx->profile,
									 device_ctx->profile_len, NULL, 0, NULL,
									 NULL);
			if (ONENET_SUCCESS == err)
			{
				os_printf("Open Device success\n");
				onenet_device_get_id_token(device_ctx->dev, &device_ctx->dev_id, device_ctx->acc_token);
				device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_LINKED;
				/** �???动mdns，允许绑定�?��??*/
				if (BIND_STATUS_UNBIND == params_get_bind_status())
					onenet_device_bind();
			}
			else
			{
				if (ONENET_DEVICE_STATUS_LINK_FAILED != device_ctx->cloud_link_status)
				{
					device_ctx->cloud_link_status =
						ONENET_DEVICE_STATUS_LINK_FAILED;
				}
			}
		}
		else if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status)
		{
			if (ONENET_UPLOAD_STATUS_WAIT_TO_SEND == device_ctx->upload_status)
			{
				err = onenet_device_send(device_ctx->dev, device_ctx->data,
										 NULL);
				if (ONENET_SUCCESS != err)
				{
					device_ctx->cloud_link_status =
						ONENET_DEVICE_STATUS_LINK_FAILED;
				}
				device_ctx->upload_status = ONENET_UPLOAD_STATUS_IDLE;
			}
			else
			{
				/** 设�?�连接�?�常时，保持心跳*/
				onenet_device_check_and_keepalive(device_ctx->dev);
			}

			/** 云�??命令处理*/
			if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status)
			{
				onenet_device_handle_downlink_data(device_ctx->dev, device_ctx,
												   onenet_device_notification_callback,
												   onenet_device_configuration_callback,
												   onenet_device_software_update_callback);
			}
		}
		/** 监测任务休眠，防止看门狗卡�??*/
		vTaskDelay(TASK_DELAY_INTERVAL_IN_MS / portTICK_RATE_MS);
	}

	onenet_device_destroy(device_ctx->dev);
}

void onenet_main_task(void *params)
{
	static uint16_t dispatch_event = STM_ENTRY_SIG;
	static uint16_t led_event = NULL;

	xQueueDevice = xQueueCreate(10, sizeof(uint8_t));

	drv_Switch_Init();

	os_timer_disarm(&dispatch_timer);
	os_timer_setfn(&dispatch_timer, (os_timer_func_t *)driver_actor, &dispatch_event);
	os_timer_arm(&dispatch_timer, 1, 0);

	os_printf("%s : %d ==========\n", __FUNCTION__, __LINE__);

	//onenet_main_running = 1;
	/** 进入主循�???*/
	while (1)
	{

		if (xQueueReceive(xQueueDevice, (void *)&onenet_main_running, (portTickType)portMAX_DELAY))
		{
			/** 等待网络连接成功，启动云�???连接*/
			if ((NETWORK_LINKED == device_ctx->module_status.net_link_flag) && (ONENET_DEVICE_STATUS_IDLE == device_ctx->cloud_link_status))
			{
				device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_START_LINK;
			}

			while (device_event)
			{
				if (device_event & SYS_TICK_EVT)
				{
					device_event &= ~SYS_TICK_EVT;
					g_sys_tick++;
					//power on counter
					os_printf("Device Tick event\n");
					onenet_app_tick(g_sys_tick);
				}

				if (device_event & STM_ENTRY_SIG)
				{
					device_event &= ~STM_ENTRY_SIG;
					led_init();
					hxd019_init();

					g_sys_tick = 0;

					dispatch_event = SYS_TICK_EVT;
					os_timer_disarm(&dispatch_timer);
					os_timer_setfn(&dispatch_timer, (os_timer_func_t *)driver_actor, &dispatch_event);
					os_timer_arm(&dispatch_timer, 1000, 1);

					led_event = DRIVER_LED_EVT;
					os_timer_disarm(&led_timer);
					os_timer_setfn(&led_timer, (os_timer_func_t *)driver_actor, &led_event);
					os_timer_arm(&led_timer, 10, 1);

					onenet_app_load();

					onenet_app_tick(0);
				}

				if (device_event & DRIVER_LED_EVT)
				{
					int sm_state = device_ctx->cloud_link_status;

					device_event &= ~DRIVER_LED_EVT;

					if (NETWORK_LINKING == device_ctx->module_status.net_link_flag)
					{
						//网络�??连接, 红灯2s一�??
						if (s_blink_cnt == 1)
						{
							led_set(RED, LED_ON);
							led_set(BLUE, LED_OFF);
						}
						else if (s_blink_cnt == 8)
						{
							led_set(RED, LED_OFF);
							led_set(BLUE, LED_OFF);
						}
						else if (s_blink_cnt >= 200)
						{
							s_blink_cnt = 0;
						}
					}
					else if (sm_state != ONENET_DEVICE_STATUS_LINK_FAILED)
					{
						//smart的不同状态下�??烁�?�率改变, 越来越快
						if (sm_state == ONENET_DEVICE_STATUS_IDLE)
						{
							s_blink_cnt_toggle = 30;
							s_blink_cnt_repeat = 60;
						}
						else if (sm_state == ONENET_DEVICE_STATUS_START_LINK)
						{
							s_blink_cnt_toggle = 20;
							s_blink_cnt_repeat = 40;
						}
						else if (sm_state == ONENET_DEVICE_STATUS_CREATED)
						{
							s_blink_cnt_toggle = 10;
							s_blink_cnt_repeat = 20;
						}
						else
						{
							s_blink_cnt_toggle = 30;
							s_blink_cnt_repeat = 60;
						}

						//smartlink, 红绿交替�??
						if (s_blink_cnt == 1)
						{
							led_set(RED, LED_ON);
							led_set(BLUE, LED_OFF);
						}
						else if (s_blink_cnt == s_blink_cnt_toggle)
						{
							led_set(RED, LED_OFF);
							led_set(BLUE, LED_ON);
						}
						else if (s_blink_cnt >= s_blink_cnt_repeat)
						{
							s_blink_cnt = 0;
						}
					}
					// else if (nw_state == NW_STATE_AP)
					// {
					// 	//ap配网状�?, 红绿一起闪�??
					// 	if (s_blink_cnt == 1)
					// 	{
					// 		led_set(RED, LED_ON);
					// 		led_set(BLUE, LED_ON);
					// 	}
					// 	else if (s_blink_cnt == 30)
					// 	{
					// 		led_set(RED, LED_OFF);
					// 		led_set(BLUE, LED_OFF);
					// 	}
					// 	else if (s_blink_cnt >= 60)
					// 	{
					// 		s_blink_cnt = 0;
					// 	}
					// }
					else
					{
						//网络连接�??
						if (device_ctx->cloud_link_status == ONENET_DEVICE_STATUS_LINKED) {
							if (hxd_get_state() == HXD_STATE_IDLE)
							{
								//hxd空闲状态，蓝色�??烁，网络已连�??
								if (s_blink_cnt == 1)
								{
									led_set(RED, LED_OFF);
									led_set(BLUE, LED_ON);
								}
								else if (s_blink_cnt == 8)
								{
									led_set(RED, LED_OFF);
									led_set(BLUE, LED_OFF);
								}
								else if (s_blink_cnt >= 200)
								{
									s_blink_cnt = 0;
								}
							}
							else
							{
								//红色常亮, hxd019正在学习
								led_set(RED, LED_ON);
								led_set(BLUE, LED_OFF);
							}
						}
						// else
						// {
						// 	// 固件升级, 100ms�??�??
						// 	if (s_blink_cnt == 1)
						// 	{
						// 		led_set(RED, LED_ON);
						// 		led_set(BLUE, LED_OFF);
						// 	}
						// 	else if (s_blink_cnt == 10)
						// 	{
						// 		led_set(RED, LED_OFF);
						// 		led_set(BLUE, LED_OFF);
						// 	}
						// 	else if (s_blink_cnt >= 20)
						// 	{
						// 		s_blink_cnt = 0;
						// 	}
						// }
					}

					// 产测模式计数�??
					if (s_manufacture_timeout)
					{
						led_set(RED, LED_ON);
						led_set(BLUE, LED_ON);
					}

					s_blink_cnt++;
				}
			}
		}
	}
}

/**
 *******************************************************************************
 * @brief       ���س�״̬������
 * @param       [in/out]  void
 * @return      void
 * @note        None
 *******************************************************************************
 */
static void Switch_LongPress_Handler( void )
{
	s_system_status.start_continue = 5;
	onenet_app_save();
	//reset system
	system_restart();

}

/**
*******************************************************************************
 * @brief       �����ʼ������?
 * @param       [in/out]  void
 * @return      void
 * @note        None
*******************************************************************************
 */
static void drv_Switch_Init( void )
{
    switch_signle = key_init_single( Reset_Pin_NUM, Reset_Pin_MUX,
                                     Reset_Pin_FUNC,
                                     &Switch_LongPress_Handler,
                                     NULL );
    switch_param.key_num = 1;
    switch_param.single_key = &switch_signle;

    key_init( &switch_param );
}

int32 onenet_start(void)
{
	int32 err = ONENET_SUCCESS;

	uart_set_print_port(UART0);

	os_printf("SDK version:%s\n", system_get_sdk_version());
	/** 初�?�化设�?�实�???*/
	if (ONENET_SUCCESS != (err = device_init(&device_ctx)))
		return err;

	/** 加载配置*/
	if (ONENET_SUCCESS != (err = params_init()))
		goto params_init_fail;

	/** 初�?�化网络*/
	network_connect(netlink_callback);

	/** 设置模组信息*/
	//set_module_info();

	/* �???动主�???�???*/
	xTaskCreate(onenet_main_task, "onenet_main_task", 1024, NULL, 2,
				NULL);

	xTaskCreate(cloud_link_task, "cloud_link_task", 1536, NULL, 2,
				NULL);
	return ONENET_SUCCESS;

params_init_fail:
	device_exit(device_ctx);
}


