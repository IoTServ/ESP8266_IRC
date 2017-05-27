/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		tlv.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 */
#ifndef __DEVICE_COMMON_H__
#define __DEVICE_COMMON_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_common.h"
#include "onenet/onenet_rgmp.h"

#ifdef _cplusplus
extern "C" {
#endif   

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/
#define ONENET_DEVICE_STATUS_IDLE	0
#define ONENET_DEVICE_STATUS_START_LINK	1
#define ONENET_DEVICE_STATUS_CREATED	2
#define ONENET_DEVICE_STATUS_LINKED		3
#define ONENET_DEVICE_STATUS_LINK_FAILED	4

#define ONENET_UPLOAD_STATUS_IDLE	0
#define ONENET_UPLOAD_STATUS_WAIT_TO_SEND	1

#define MODULE_STATUS_IDLE	0
#define MODULE_STATUS_SERIAL_INIT	1

#define ACCESS_TOKEN_LEN_MAX	32
#define NOTIFICATION_UUID_LEN_MAX	40
#define SENSOR_DATA_NUM_MAX	1

#define DEVICE_NAME_LEN_MAX		32
#define PRODUCT_UUID_LEN_MAX		64
#define PRODUCT_REGISTER_CODE_LEN_MAX	32
#define PRODUCT_IDENTIFIER_LEN_MAX	16
#define PRODUCT_VERSION_LEN_MAX		16

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
struct module_status {
	uint8 net_link_flag :1;
	uint8 cloud_link_flag :1;
	uint8 bind_status :1;
	uint8 smartconfig_mode :2;
	uint8 test_mode :1;
	uint8 reserved_1 :2;

	uint8 rssi;
} __packed;

struct device {
	/** 云端设备属性、数据*/
	void *dev;				//用于和云端通信的设备实例
	uint32 cloud_link_status;		//设备与云端的连接状态
	sint64 dev_id;				//云端分配的设备ID
	uint8 acc_token[ACCESS_TOKEN_LEN_MAX];	//用于设备绑定等功能的鉴权
	rgmp_notification_id_t *ntf;		//云端下发的通知标识
	rgmp_uplink_data_t *data;		//需要上传到云端的数据
	//rgmp_uplink_result_t *data_result;	//上传数据的返回结果
	uint8 register_code[PRODUCT_REGISTER_CODE_LEN_MAX];		//
	uint8 product_uuid[PRODUCT_UUID_LEN_MAX];
	uint8 product_ident[PRODUCT_IDENTIFIER_LEN_MAX];
	uint8 product_ver[PRODUCT_VERSION_LEN_MAX];
	uint8 *profile;
	uint32 profile_len;

	/** 本地运行属性*/
	struct module_status module_status;	//无线模组状态
	uint8 status_changed;				//模组状态改变，需要通知MCU
	uint8 wait_ack;						//是否有REQUEST在等待响应
	uint8 current_request;				//当前在等待响应的请求号
	uint8 upload_status;				//数据上传状态
};
/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/
rgmp_notification_id_t* device_ntf_id_malloc(void);
void device_ntf_id_free(rgmp_notification_id_t *ntf_id);
rgmp_uplink_data_t* device_data_malloc(void);
void device_data_free(rgmp_uplink_data_t *data);
int32 device_init(struct device **dev_ctx);

#ifdef _cplusplus
}
#endif   

#endif

