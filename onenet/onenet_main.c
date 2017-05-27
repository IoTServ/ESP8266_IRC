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

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define MODULE_VER_MAJOR	"1"
#define MODULE_VER_MINOR	"0"

#define TASK_DELAY_INTERVAL_IN_MS	1000
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct module_time {
	uint8 valid;
	uint8 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minite;
	uint8 second;
};

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

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* External Functions and Variables                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

static void netlink_callback(enum network_status status)
{
	os_printf("%s data changed %d -> %d\n", __FUNCTION__, device_ctx->module_status.net_link_flag, status);
	if(status != device_ctx->module_status.net_link_flag)
	{
		if((ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status) && (NETWORK_LINKING == status))
			device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_LINK_FAILED;
	}
	device_ctx->module_status.net_link_flag = status;
}

static int onenet_device_send_data(uint8 *data, uint16 data_len)
{
	int32 ret = ONENET_SUCCESS;

	if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status) {
		if (ONENET_UPLOAD_STATUS_IDLE == device_ctx->upload_status) {
			device_ctx->data->sensor_data[0].data = data;
			device_ctx->data->sensor_data[0].len = data_len;
			device_ctx->data->sensor_data[0].timestamp = 0;
			device_ctx->data->sd_count = 1;
			os_printf("Send Data ====\n");
			device_ctx->upload_status =
			ONENET_UPLOAD_STATUS_WAIT_TO_SEND;
		} else
			ret = ONENET_RESOURCE_BUSY;
	} else {
		/** 返回错误*/
		ret = ONENET_DEVICE_OFFLINE;
	}

	return ret;
}

static int onenet_device_send_event(uint8 *data, uint16 data_len)
{
	int32 ret = ONENET_SUCCESS;

	if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status) {
		if (ONENET_UPLOAD_STATUS_IDLE == device_ctx->upload_status) {
			device_ctx->data->event.event = data + 1;
			device_ctx->data->event.len = data_len - 1;
			device_ctx->data->event.timestamp = 0;
			device_ctx->data->event.level = *data;
			device_ctx->upload_status =
			ONENET_UPLOAD_STATUS_WAIT_TO_SEND;
		} else
			ret = ONENET_RESOURCE_BUSY;
	} else {
		/** 返回错误*/
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
	sprintf(dev_name, "%s_%s", device_ctx->product_ident,
			params_get_unique_id());
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
	/** 仅记录固件包信息，由其它任务来执行升级*/

}

static int onenet_device_configuration_callback(void *context,
		const char *configuration, size_t size)
{
	/** 处理下发的配置，格式为tlv*/
}

static void cloud_link_task(void *params)
{
	int32 err = 0;

	cloud_link_running = 1;

	while (cloud_link_running) {
		/** 初始状态时，创建设备*/
		if (ONENET_DEVICE_STATUS_START_LINK == device_ctx->cloud_link_status) {
			os_printf("Create Device\n");
			err = onenet_device_create(device_ctx->product_uuid,
					device_ctx->register_code, &(device_ctx->dev));
			if (ONENET_SUCCESS == err) {
				os_printf("Create Device success\n");
				device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_CREATED;
			}
		}

		/** 设备创建成功后，或设备掉线时，重新打开设备进行网络重连*/
		if ((ONENET_DEVICE_STATUS_CREATED == device_ctx->cloud_link_status)
				|| (ONENET_DEVICE_STATUS_LINK_FAILED
						== device_ctx->cloud_link_status)) {
			os_printf("Open Device \n");
			err = onenet_device_open(device_ctx->dev, device_ctx->profile,
					device_ctx->profile_len, NULL, 0, NULL,
					NULL);
			if (ONENET_SUCCESS == err) {
				os_printf("Open Device success\n");
				onenet_device_get_id_token(device_ctx->dev, &device_ctx->dev_id, device_ctx->acc_token);
				device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_LINKED;
				/** 启动mdns，允许绑定设备*/
				if(BIND_STATUS_UNBIND == params_get_bind_status())
					onenet_device_bind();
			} else {
				if (ONENET_DEVICE_STATUS_LINK_FAILED
						!= device_ctx->cloud_link_status) {
					device_ctx->cloud_link_status =
					ONENET_DEVICE_STATUS_LINK_FAILED;
				}
			}
		} else if (ONENET_DEVICE_STATUS_LINKED
				== device_ctx->cloud_link_status) {
			if (ONENET_UPLOAD_STATUS_WAIT_TO_SEND
					== device_ctx->upload_status) {
				err = onenet_device_send(device_ctx->dev, device_ctx->data,
				NULL);
				if (ONENET_SUCCESS != err) {
					device_ctx->cloud_link_status =
					ONENET_DEVICE_STATUS_LINK_FAILED;
				}
				device_ctx->upload_status = ONENET_UPLOAD_STATUS_IDLE;
			} else {
				/** 设备连接正常时，保持心跳*/
				onenet_device_check_and_keepalive(device_ctx->dev);
			}

			/** 云端命令处理*/
			if (ONENET_DEVICE_STATUS_LINKED == device_ctx->cloud_link_status) {
				onenet_device_handle_downlink_data(device_ctx->dev, device_ctx,
						onenet_device_notification_callback,
						onenet_device_configuration_callback,
						onenet_device_software_update_callback);

			}
		}
		/** 监测任务休眠，防止看门狗卡死*/
		vTaskDelay(TASK_DELAY_INTERVAL_IN_MS / portTICK_RATE_MS);
	}

	onenet_device_destroy(device_ctx->dev);
}

void onenet_main_task(void *params)
{
	uint16 recv_data_len = 0;

	os_printf("%s : %d ==========\n", __FUNCTION__, __LINE__);

	onenet_main_running = 1;
	/** 进入主循环*/
	while (onenet_main_running) {
		/** 等待网络连接成功，启动云端连接*/
		if((NETWORK_LINKED == device_ctx->module_status.net_link_flag) && (ONENET_DEVICE_STATUS_IDLE == device_ctx->cloud_link_status))
			device_ctx->cloud_link_status = ONENET_DEVICE_STATUS_START_LINK;

		/** 用户循环处理*/

		/** 休眠防卡死*/
		vTaskDelay(TASK_DELAY_INTERVAL_IN_MS / portTICK_RATE_MS);
	}
}


int32 onenet_start(void)
{
	int32 err = ONENET_SUCCESS;

	uart_set_print_port(UART1);

	os_printf("SDK version:%s\n", system_get_sdk_version());
	/** 初始化设备实例*/
	if (ONENET_SUCCESS != (err = device_init(&device_ctx)))
		return err;

	/** 加载配置*/
	if (ONENET_SUCCESS != (err = params_init()))
		goto params_init_fail;

	/** 初始化网络*/
	network_connect(netlink_callback);

	/** 设置模组信息*/
	//set_module_info();

	/* 启动主循环*/
	xTaskCreate(onenet_main_task, "onenet_main_task", 1024, NULL, 2,
							NULL);

	xTaskCreate(cloud_link_task, "cloud_link_task", 1536, NULL, 2,
								NULL);
	return ONENET_SUCCESS;

	params_init_fail: device_exit(device_ctx);
}
