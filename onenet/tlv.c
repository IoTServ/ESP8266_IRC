/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		tlv.c
 * @brief 
 * @author 		宋伟<songwei1@iot.chinamobile.com>
 * @date 		2017/02/16
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 * 		2017/02/16		宋伟		  1.0.0			file created
 */

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "esp_common.h"
#include "onenet_err.h"
#include "tlv.h"

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
int32 tlv_push_in(struct tlv_translator *tlv, enum tlv_type type,
		const uint8 *data, uint16 data_len)
{
	if (NULL == tlv)
		return ONENET_INVALID_PARAM;

	tlv->buf[tlv->offset++] = type;
	tlv->buf[tlv->offset++] = (data_len & 0xFF);
	tlv->buf[tlv->offset++] = ((data_len >> 8) & 0xFF);
	if (data_len > 0) {
		memcpy(tlv->buf + tlv->offset, data, data_len);
		tlv->offset += data_len;
	}

	return ONENET_SUCCESS;
}

uint32 tlv_get_next_pool_len(struct tlv_translator *tlv)
{
	uint32 len = tlv->buf[tlv->offset + 1] | (tlv->buf[tlv->offset + 2]);

	return len;
}

int32 tlv_pool_out(struct tlv_translator *tlv, uint8 *data, uint16 *data_len)
{
	enum tlv_type type = TLV_TYPE_INT;

	if ((NULL == tlv) || (NULL == data))
		return ONENET_INVALID_PARAM;
	if (tlv->offset + 3 > tlv->buf_len)
		return ONENET_IO_EOF;
	type = tlv->buf[tlv->offset++];
	if ((type < TLV_TYPE_INT) || (type > TLV_TYPE_BYTE))
		return ONENET_INVALID_PARAM;

	*data_len = tlv->buf[tlv->offset++] | (tlv->buf[tlv->offset++] << 8);
	if ((*data_len) && (*data_len + tlv->offset <= tlv->buf_len))
		memcpy(data, tlv->buf + tlv->offset, *data_len);
	tlv->offset += *data_len;

	return ONENET_SUCCESS;
}

struct tlv_translator* tlv_create(uint8 *buf, uint16 buf_len)
{
	struct tlv_translator *tlv = NULL;

	if ((NULL == buf) && (0 == buf_len))
		return NULL ;

	tlv = malloc(sizeof(*tlv));
	if (tlv) {
		memset(tlv, 0, sizeof(*tlv));
		if (NULL != buf) {
			tlv->buf = buf;
		} else {
			tlv->buf = malloc(buf_len);
			if (NULL == tlv->buf) {
				free(tlv);
				return NULL ;
			}
			tlv->buf_inside = 1;
		}
		tlv->buf_len = buf_len;
		return tlv;
	}

	return NULL ;
}

void tlv_delete(struct tlv_translator *tlv)
{
	if (tlv) {
		if ((1 == tlv->buf_inside) && (NULL != tlv->buf))
			free(tlv->buf);
		free(tlv);
	}
}
