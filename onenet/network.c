/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		network.c
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "esp_common.h"

#include "onenet_err.h"
#include "params.h"
#include "network.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
network_callback_t net_cb = NULL;

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* External Functions and Variables                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
static void network_event_handle(System_Event_t *evt)
{
	switch (evt->event_id) {
		case EVENT_STAMODE_GOT_IP:
			if (net_cb)
				net_cb(NETWORK_LINKED);
			break;
		case EVENT_STAMODE_DISCONNECTED:
			if(net_cb)
				net_cb(NETWORK_LINKING);
			break;
		default:
			break;
	}
}

static void network_smartconfig_done(sc_status status, void *pdata)
{
	switch (status) {
		case SC_STATUS_LINK_OVER:
			smartconfig_stop();
			break;
		case SC_STATUS_LINK:
		{
			struct station_config *sta_conf = pdata;

			wifi_station_set_config(sta_conf);
			wifi_station_disconnect();
			wifi_station_connect();
		}
			break;
		default:
			break;
	}
}

int32 network_connect(network_callback_t cb)
{
	net_cb = cb;

	/** 注册网络事件通知回调*/
	wifi_set_event_handler_cb(network_event_handle);

	/** �?动连�?*/
	if (true == wifi_station_connect())
		return ONENET_SUCCESS;
	else
		return ONENET_NETLINK_FAILED;
}

void network_disconnect(void)
{
	smartconfig_stop();
	wifi_station_disconnect();
	wifi_set_event_handler_cb(NULL);
}

int32 network_start_smartconfig(uint8 type)
{
	os_printf("s_manufacture_mode12\r\n");
	params_set_smartconfig_type(type);
	os_printf("s_manufacture_mode13\r\n");
	smartconfig_stop();
	os_printf("s_manufacture_mode14\r\n");
	if (SMARTCONFIG_TYPE_SOFTAP == type) {
		wifi_set_opmode(SOFTAP_MODE);
	} else {
		wifi_set_opmode(STATION_MODE);
		os_printf("smartconfig type %d\n", type);
		smartconfig_set_type(type);
		smartconfig_start(network_smartconfig_done);
	}

	return ONENET_SUCCESS;
}

int32 network_get_addr(struct network_info *info)
{
	WIFI_INTERFACE if_mode = STATION_IF;

	if (SMARTCONFIG_TYPE_SOFTAP == params_get_smartconfig_type())
		if_mode = SOFTAP_IF;

	if (true == wifi_get_ip_info(if_mode, (struct ip_info *) info))
		return ONENET_SUCCESS;
	else
		return ONENET_SUCCESS;
}

int8 network_get_rssi(void)
{
	return wifi_station_get_rssi();
}
