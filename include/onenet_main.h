/**
 * Copyright (c), 2012~2017 iot.10086.cn All Rights Reserved
 * @file 		onenet_main.h
 * @brief 
 * @author 		瀹嬩紵<songwei1@iot.chinamobile.com>
 * @date
 * @version 	1.0.0
 * @par Revision History:
 * 		Date			Author		Revised.Ver		Notes
 */
#ifndef __ONENET_MAIN_H__
#define __ONENET_MAIN_H__

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

#ifdef _cplusplus
extern "C"{
#endif   


/*****************************************************************************/
/* External Definition锛圕onstant and Macro )                                 */
/*****************************************************************************/
// FLASH MAP
// 0x7c: SAVE1 0x7d: SAVE2 0x7e: FLAG
#define ONENET_APP_START   			0xFA

#define APP_FLASH_SEC_SIZE  		4096

#define ONENET_APP_START_SEC 		(ONENET_APP_START * APP_FLASH_SEC_SIZE)

#define MODULE_VER_MAJOR 	"1"
#define MODULE_VER_MINOR 	"0"

#define ONENET_REG_CODE			"NEylSkg61m04effH"
#define ONENET_PRODUCT_IDENT	"IrControl"
#define ONENET_PRODUCT_UUID		"b4a7c5bc-fba8-5f9d-a8a6-6f18131e3098"

#define IRCODE_LEN  48

typedef struct mcu_status_t
{
	uint8 ircode[IRCODE_LEN];
	uint8 ircode_save[IRCODE_LEN];
} __attribute__((aligned(1), packed)) mcu_status_t;

/*
 * 属性标记位为1表示该属性值有效，0为无效
 * mcu_attr_flags指示哪个属性被改动过了，则设置mcu相应的mcu_status
 */
typedef struct mcu_attr_flags_t
{
	uint8 setting_ircode: 1;
	uint8 reserved: 7;
} __attribute__((aligned(1), packed)) mcu_attr_flags_t;

typedef struct system_status_t
{
	uint8 init_flag;
	uint16 start_count;
	uint8 start_continue;
	mcu_status_t mcu_status;
} __attribute__((aligned(4), packed)) system_status_t;




/*****************************************************************************/
/* External Structures, Enum and Typedefs                                    */
/*****************************************************************************/

/*****************************************************************************/
/* External Function Prototypes                                              */
/*****************************************************************************/
void onenet_cloud_receive(const unsigned char *pdata, unsigned short length);
void onenet_app_apply_settings(void);
void onenet_app_load(void);
void onenet_app_save(void);
void onenet_app_tick(uint32_t tick);
void onenet_app_send(char *s);




int32 onenet_start(void);

#ifdef _cplusplus
}
#endif   

#endif

