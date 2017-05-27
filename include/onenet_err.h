/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		onenet_err.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/01/18
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/01/18		宋伟		  1.0.0			file created
 */
#ifndef __ONENET_ERR_H__
#define __ONENET_ERR_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

#ifdef _cplusplus
extern "C"{
#endif   

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/
#define ONENET_SUCCESS		0
#define ONENET_NOMEM		-1
#define ONENET_RESOURCE_BUSY	-2
#define ONENET_TIMEOUT		-3
#define ONENET_INVALID_PARAM	-4
#define ONENET_IO_ERR		-5
#define ONENET_UNINITIALIZED	-6
#define ONENET_MULTI_INITTIALIZED	-7
#define ONENET_NO_TARGET	-8
#define ONENET_GET_FAILED	-9
#define ONENET_NETLINK_FAILED	-10
#define ONENET_GET_NETINFO_FAILED	-11
#define ONENET_IO_EOF	-12
#define ONENET_DEVICE_OFFLINE	-13




/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/


#ifdef _cplusplus
}
#endif   

#endif

