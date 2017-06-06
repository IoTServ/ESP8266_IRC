/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		params.c
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
#include "spi_flash.h"
#include "mbedtls/base64.h"

#include "onenet_err.h"
#include "params.h"
#include "common_def.h"

/*****************************************************************************/
/* Local Definitions ( Constant and Macro )                                  */
/*****************************************************************************/
#define PARAMS_SECTOR_START	0xFB
#define PARAMS_SECTOR_NUM	1

#define PARAMS_ADDR_START (PARAMS_SECTOR_START * SPI_FLASH_SEC_SIZE)

#define PARAM_FILE_HEADER_MASK	0x55AA
#define MD5_LENGTH 16
/*****************************************************************************/
/* Structures, Enum and Typedefs                                             */
/*****************************************************************************/
struct params {
	/** 通用配置*/
	uint8 bind_status;
	/** 模组参数配置*/
	uint8 smartconfig_type;

#if 0
	/** 云�??参数配置*/
	uint8 device_name[DEVICE_NAME_LEN_MAX];
	uint8 product_ident[PRODUCT_IDENTIFIER_LEN_MAX];
	uint8 product_ver[PRODUCT_VERSION_LEN_MAX];
	uint8 product_uuid[PRODUCT_UUID_LEN_MAX];
	uint8 register_code[PRODUCT_REGISTER_CODE_LEN_MAX];
#endif
}__packed;

struct file_header {
	unsigned short header_mask;
	unsigned short file_len;
	char md5[MD5_LENGTH];
};
/*****************************************************************************/
/* Local Function Prototype                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static struct params *user_params = NULL;
static uint8 unique_id[32] = { 0 };

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* External Functions and Variables                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/
#if 0
static int32 params_check(void)
{
	if(NULL == user_params)
	return ONENET_UNINITIALIZED;

	return ONENET_SUCCESS;
}
#endif

#if 0
static int32 params_check_buf(const uint8 *buf)
{
	if (NULL == user_params)
	return ONENET_UNINITIALIZED;

	if (NULL == buf)
	return ONENET_INVALID_PARAM;

	return ONENET_SUCCESS;
}

static void params_set_string(uint8 *dst, uint16 dst_buf_len, const uint8 *src)
{
	memset(dst, 0, dst_buf_len);
	strcpy(dst, src);
}

static const uint8* check_and_return_buf(uint8 *buf)
{
	if (strlen(buf))
	return buf;
	else
	return NULL;
}
#endif

static int32 params_save(void)
{
#if 0
	uint16 encoded_len = 0;
	uint16 real_len = 0;
	uint8 *buf = NULL;
	int err = 0;

	/** 长度为n的数据Base64编码后的长度为[n/3]*4，[]表示向上取整*/
	encoded_len = (sizeof(*user_params) / 3 + (sizeof(*user_params) % 3 ? 1 : 0)) * 4;
	buf = os_malloc(encoded_len + 1);
	if (NULL == buf) {
		return;
	}

	memset(buf, 0, encoded_len + 1);
	err = mbedtls_base64_encode((unsigned char *) buf, encoded_len + 1, &real_len, (uint8 *) user_params,
			sizeof(*user_params));

	if ((0 == err) && (real_len <= SPI_FLASH_SEC_SIZE)) {
		spi_flash_erase_sector(PARAMS_SECTOR_START);
		spi_flash_write(PARAMS_ADDR_START, (uint32 *) buf, real_len);
	}
	free(buf);
#else
	struct file_header header;

	header.header_mask = PARAM_FILE_HEADER_MASK;
	header.file_len = sizeof(struct params);
	mbedtls_md5(user_params, header.file_len, header.md5);

	if ((SPI_FLASH_RESULT_OK != spi_flash_erase_sector(PARAMS_SECTOR_START))
			|| (SPI_FLASH_RESULT_OK
					!= spi_flash_write(PARAMS_ADDR_START, (uint32 *) &header,
							sizeof(header)))
			|| (SPI_FLASH_RESULT_OK
					!= spi_flash_write(PARAMS_ADDR_START + sizeof(header),
							(uint32 *) user_params, header.file_len)))
		return ONENET_IO_ERR;
#endif
}

static int32 params_load_default(void)
{
	user_params->bind_status = BIND_STATUS_UNBIND;
	user_params->smartconfig_type = SMARTCONFIG_TYPE_ESPTOUCH;
}

static int32 params_load(void)
{
#if 0
	uint16 encoded_len = 0;
	uint8 *buf = NULL;

	if (NULL == user_params)
	return ONENET_UNINITIALIZED;

	encoded_len = (sizeof(*user_params) / 3 + (sizeof(*user_params) % 3 ? 1 : 0)) * 4;
	buf = os_malloc(encoded_len + 1);
	if (NULL == buf)
	return ONENET_NOMEM;

	memset(buf, 0, encoded_len + 1);
	if (SPI_FLASH_RESULT_OK == spi_flash_read(PARAMS_ADDR_START, (uint32 *) buf, encoded_len)) {
		size_t real_len = 0;
		int err = 0;
		err = mbedtls_base64_decode((unsigned char *) user_params, sizeof(*user_params), &real_len, buf,
				encoded_len);
		free(buf);
		if ((0 == err) && (real_len == sizeof(*user_params)))
		return ONENET_SUCCESS;
		else
		return ONENET_NO_TARGET;
	}
	return ONENET_IO_ERR;
#else
	char tmp_md5[MD5_LENGTH] = { 0 };
	struct file_header header;

	if ((SPI_FLASH_RESULT_OK
			!= spi_flash_read(PARAMS_ADDR_START, (uint32 *) &header,
					sizeof(header))
			|| (PARAM_FILE_HEADER_MASK != header.header_mask)))
		return ONENET_IO_ERR;

	if ((SPI_FLASH_RESULT_OK
			== spi_flash_read(PARAMS_ADDR_START + sizeof(header),
					(uint32 *) &user_params, header.file_len))) {
		mbedtls_md5(&user_params, header.file_len, tmp_md5);
		if (0 == memcmp(tmp_md5, header.md5, MD5_LENGTH)) {
			return ONENET_SUCCESS;
		}
	}
	return ONENET_IO_ERR;
#endif
}

int32 params_set_bind_status(uint8 bind_status)
{
	if (NULL == user_params)
		return ONENET_UNINITIALIZED;

	user_params->bind_status = bind_status;
}

int32 params_get_bind_status(void)
{
	if (NULL == user_params)
		return ONENET_UNINITIALIZED;

	return user_params->bind_status;
}

int32 params_set_smartconfig_type(uint8 type)
{
	if (NULL == user_params)
		return ONENET_UNINITIALIZED;

	user_params->smartconfig_type = type;
	os_printf("Set smartconfig %d\n", type);
}

int32 params_get_smartconfig_type(void)
{
	if (NULL == user_params)
		return ONENET_UNINITIALIZED;

	os_printf("Get smartconfig %d\n", user_params->smartconfig_type);
	return user_params->smartconfig_type;
}

#if 0
int32 params_set_test_mode(uint8 test_mode);
int32 params_get_test_mode(void);
#endif

#if 0
int32 params_set_device_name(const uint8 *name)
{
	int32 err = params_check_buf(name);

	if (ONENET_SUCCESS != err)
	return err;

	params_set_string(user_params->device_name,
			sizeof(user_params->device_name), name);
	return ONENET_SUCCESS;
}

const uint8* params_get_device_name(void)
{
	return check_and_return_buf(user_params->device_name);
}

int32 params_set_product_identifier(const uint8 *ident)
{
	int32 err = params_check_buf(ident);

	if (ONENET_SUCCESS != err)
	return err;

	params_set_string(user_params->product_ident,
			sizeof(user_params->product_ident), ident);
	return ONENET_SUCCESS;
}

const uint8* params_get_product_identifier(void)
{
	return check_and_return_buf(user_params->product_ident);
}

int32 params_set_product_version(const uint8 *ver)
{
	int32 err = params_check_buf(ver);

	if (ONENET_SUCCESS != err)
	return err;

	params_set_string(user_params->product_ver,
			sizeof(user_params->product_ver), ver);
	return ONENET_SUCCESS;
}

const uint8* params_get_product_version(void)
{
	return check_and_return_buf(user_params->product_ver);
}

int32 params_set_product_uuid(const uint8 *uuid)
{
	int32 err = params_check_buf(uuid);

	if (ONENET_SUCCESS != err)
	return err;

	params_set_string(user_params->product_uuid,
			sizeof(user_params->product_uuid), uuid);
	return ONENET_SUCCESS;
}

const uint8* params_get_product_uuid(void)
{
	return check_and_return_buf(user_params->product_uuid);
}

int32 params_set_register_code(const uint8 *register_code)
{
	int32 err = params_check_buf(register_code);

	if (ONENET_SUCCESS != err)
	return err;

	params_set_string(user_params->register_code,
			sizeof(user_params->register_code), register_code);
	return ONENET_SUCCESS;
}

const uint8* params_get_register_code(void)
{
	return check_and_return_buf(user_params->register_code);
}
#endif

const uint8* params_get_unique_id(void)
{
	if (0 == strlen(unique_id)) {
		sprintf(unique_id, "%x", system_get_chip_id());
	}
	return unique_id;
}

int32 params_init(void)
{
	int32 err = ONENET_SUCCESS;

	if (NULL == user_params) {
		user_params = os_malloc(sizeof(*user_params));
		if (NULL == user_params)
			return ONENET_NOMEM;
	} else
		return ONENET_MULTI_INITTIALIZED;

	memset((void *) user_params, 0, sizeof(*user_params));
	err = params_load();
	if (ONENET_IO_ERR == err) {
		params_load_default();
		return ONENET_SUCCESS;
	} else
		return err;
}

int32 params_exit(void)
{
	if (user_params) {
		params_save();
		free(user_params);
	}
}
