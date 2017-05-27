/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		device_bind.c
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/01/22
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/01/22		宋伟		  1.0.0			file created
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "esp_common.h"
#include "lwip/mdns.h"

#include "onenet_err.h"
#include "params.h"
#include "network.h"
#include "device_bind.h"

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

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* External Functions and Variables                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
int32 device_bind_start(sint64 dev_id, const uint8 *token,
		const uint8 *dev_name, const uint8 *product_ident)
{
	struct mdns_info info;
	struct network_info net_info;
	uint32 i = 0;

	/* 清掉设备绑定标识*/
	params_set_bind_status(BIND_STATUS_UNBIND);

	/* 设定mDNS参数*/
	memset(&info, 0, sizeof(info));
	info.host_name = (uint8 *) dev_name;
	info.server_name = (uint8 *) product_ident;
	network_get_addr(&net_info);
	info.ipAddr = net_info.ip;
	info.server_port = 0;

	info.txt_data[0] = os_malloc(20);
	memset(info.txt_data[0], 0, 20);
	sprintf(info.txt_data[0], "%d", dev_id);

	info.txt_data[1] = (uint8 *) token;
	os_printf("id %s  token %s\n", info.txt_data[0], info.txt_data[1]);
	//info.txt_data[2] = (uint8 *) params_get_product_version();

	/* 启动mDNS服务，等待绑定*/
	mdns_init(&info);

	if (info.txt_data[0])
		os_free(info.txt_data[0]);

	return ONENET_SUCCESS;
}

int32 device_bind_stop(void)
{
	mdns_close();
	return ONENET_SUCCESS;
}

int32 device_unbind(void)
{
	/* 向云端发送解绑请求*/

	/* 清掉设备绑定标识*/
	params_set_bind_status(BIND_STATUS_BINDED);

	return ONENET_SUCCESS;
}
