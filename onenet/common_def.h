/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		common_def.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 */
#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "smartconfig.h"

#ifdef _cplusplus
extern "C" {
#endif   

	/*****************************************************************************/
	/* External Definition（Constant and Macro )                                 */
	/*****************************************************************************/
	/** 智能配网(SmartConfig 模式)*/
#define	SMARTCONFIG_TYPE_ESPTOUCH		SC_TYPE_ESPTOUCH
#define	SMARTCONFIG_TYPE_AIRKISS		SC_TYPE_AIRKISS
#define	SMARTCONFIG_TYPE_ESPTOUCH_AIRKISS	SC_TYPE_ESPTOUCH_AIRKISS
#define SMARTCONFIG_TYPE_SOFTAP			4

	/** 设备绑定状态*/
#define BIND_STATUS_UNBIND	0
#define BIND_STATUS_BINDED	1

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

