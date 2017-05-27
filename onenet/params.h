/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		params.h
 * @brief 		参数管理模块，不负责检查参数有效性
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/01/22
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/01/22		宋伟		  1.0.0			file created
 */
#ifndef __PARAMS_H__
#define __PARAMS_H__

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
int32 params_set_bind_status(uint8 bind_status);
int32 params_get_bind_status(void);

int32 params_set_smartconfig_type(uint8 type);
int32 params_get_smartconfig_type(void);

#if 0
int32 params_set_test_mode(uint8 test_mode);
int32 params_get_test_mode(void);
#endif

#if 0
int32 params_set_device_name(const uint8 *name);
const uint8* params_get_device_name(void);

int32 params_set_product_identifier(const uint8 *ident);
const uint8* params_get_product_identifier(void);

int32 params_set_product_version(const uint8 *ver);
const uint8* params_get_product_version(void);

int32 params_set_product_uuid(const uint8 *uuid);
const uint8* params_get_product_uuid(void);

int32 params_set_register_code(const uint8 *register_code);
const uint8* params_get_register_code(void);
#endif

const uint8* params_get_unique_id(void);

int32 params_init(void);
int32 params_exit(void);

#ifdef _cplusplus
}
#endif   

#endif

