/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		tlv.h
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/02/16
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/02/16		宋伟		  1.0.0			file created
 */
#ifndef __TLV_H__
#define __TLV_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "c_types.h"

#ifdef _cplusplus
extern "C" {
#endif   

/*****************************************************************************/
/* External Definition（Constant and Macro )                                 */
/*****************************************************************************/

/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/
struct tlv_translator {
	uint8 buf_inside;
	uint8 *buf;
	uint16 buf_len;
	uint16 offset;
};

enum tlv_type {
	TLV_TYPE_INT = 1,
	TLV_TYPE_LONG,
	TLV_TYPE_DOUBLE,
	TLV_TYPE_BOOL,
	TLV_TYPE_STRING,
	TLV_TYPE_BYTE
};
/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/
int32 tlv_push_in(struct tlv_translator *tlv, enum tlv_type type,
		const uint8 *data, uint16 data_len);
int32 tlv_pool_out(struct tlv_translator *tlv, uint8 *data, uint16 *data_len);
uint32 tlv_get_next_pool_len(struct tlv_translator *tlv);
struct tlv_translator* tlv_create(uint8 *buf, uint16 buf_len);
void tlv_delete(struct tlv_translator *tlv);

#ifdef _cplusplus
}
#endif   

#endif

