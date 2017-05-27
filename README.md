ESP8266_oneNET_SOC
==================
# 移植步骤
1. 完善onenet/onenet_main.c/onenet_start()中的模组设置信息
```c
set_module_info();
```

2. 实现云端下发数据处理回调
```c
static void onenet_device_notification_callback(void *context, const rgmp_notification_id_t *id, const char *notification, size_t size);
static void onenet_device_software_update_callback(void *context, const rgmp_module_update_info_t *modules, int count);
static int onenet_device_configuration_callback(void *context, const char *configuration, size_t size);
```
software_update暂不支持，可以不实现。传入的数据(notification/configuration)均为tlv格式

3. 需要上传数据时调用，数据格式为tlv
```c
static int onenet_device_send_data(uint8 *data, uint16 data_len);
```
4. 需要上传事件时调用，数据格式为tlv
```c
static int onenet_device_send_event(uint8 *data, uint16 data_len);
```
