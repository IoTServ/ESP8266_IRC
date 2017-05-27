/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		device_bind.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/01/22
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/01/22		宋伟		  1.0.0			file created
 */
#ifndef __DEVICE_BIND_H__
#define __DEVICE_BIND_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

#ifdef _cplusplus
extern "C" {
#endif   

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/
int32 device_bind_start(sint64 dev_id, const uint8 *token,
		const uint8 *dev_name, const uint8 *product_ident);
int32 device_bind_stop(void);
int32 device_unbind(void);

#ifdef _cplusplus
}
#endif   

#endif

