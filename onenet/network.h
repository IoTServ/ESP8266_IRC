/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		network.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 */
#ifndef __NETWORK_H__
#define __NETWORK_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "common_def.h"

#ifdef _cplusplus
extern "C" {
#endif   

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
enum network_status {
	NETWORK_LINKING = 0,
	NETWORK_LINKED
};

struct network_info {
	u32 ip;
	u32 netmask;
	u32 gw;
};

typedef void (*network_callback_t)(enum network_status status);

/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/
int32 network_connect(network_callback_t cb);
void network_disconnect(void);
int32 network_start_smartconfig(uint8 type);
int32 network_get_addr(struct network_info *info);
int8 network_get_rssi(void);

#ifdef _cplusplus
}
#endif   

#endif

