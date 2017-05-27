/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		device_common.c
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
#include "device_common.h"
#include "onenet_err.h"

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

void device_ntf_id_free(rgmp_notification_id_t *ntf_id)
{
	if (ntf_id) {
		if (ntf_id->uuid)
			os_free(ntf_id->uuid);
		os_free(ntf_id);
	}
}

rgmp_notification_id_t* device_ntf_id_malloc(void)
{
	rgmp_notification_id_t *ntf = NULL;

	ntf = (rgmp_notification_id_t *) os_malloc(sizeof(rgmp_notification_id_t));
	if (ntf) {
		memset(ntf, 0, sizeof(rgmp_notification_id_t));
		ntf->uuid = os_malloc(NOTIFICATION_UUID_LEN_MAX);
		if (ntf->uuid) {
			memset(ntf->uuid, 0, NOTIFICATION_UUID_LEN_MAX);
		} else {
			os_free(ntf);
			ntf = NULL;
		}
	}
	return ntf;
}

void device_data_free(rgmp_uplink_data_t *data)
{
	if (data) {
		if (data->sensor_data)
			os_free(data->sensor_data);
		os_free(data);
	}
}

rgmp_uplink_data_t* device_data_malloc(void)
{
	rgmp_uplink_data_t *data = NULL;

	data = os_malloc(sizeof(rgmp_uplink_data_t));
	if (data) {
		memset(data, 0, sizeof(rgmp_uplink_data_t));
		data->sensor_data = os_malloc(sizeof(rgmp_sensor_data_t));
		if (data->sensor_data) {
			memset(data->sensor_data, 0, sizeof(rgmp_sensor_data_t));
		} else {
			os_free(data);
			data = NULL;
		}
	}

	return data;
}

void device_exit(struct device *dev)
{
	if (dev) {
		device_ntf_id_free(dev->ntf);
		device_data_free(dev->data);
		os_free(dev);
	}
}

int32 device_init(struct device **dev_ctx)
{
	struct device *dev = NULL;

	if (NULL == dev) {
		dev = os_malloc(sizeof(*dev));
		if (NULL != dev) {
			memset(dev, 0, sizeof(*dev));
			if ((NULL == (dev->ntf = device_ntf_id_malloc()))
					|| (NULL == (dev->data = device_data_malloc()))) {
				device_ntf_id_free(dev->ntf);
				device_data_free(dev->data);
				os_free(dev);
				dev = NULL;
			} else
			{
				*dev_ctx = dev;
				return ONENET_SUCCESS;
			}
		}
	}
	return ONENET_NOMEM;
}
