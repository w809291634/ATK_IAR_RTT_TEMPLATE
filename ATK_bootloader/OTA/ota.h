#ifndef __OTA_H__
#define __OTA_H__
#include "sys.h"
#include "esp32_at.h"

/** ·þÎñÆ÷ **/
extern char IOT_PRO_ID_NAME[];
extern char IOT_SERVER_IP[];
#define IOT_SERVER_PORT         80

/** º¯Êý **/
int http_data_handle(char *buf,uint16_t len);
void OTA_system_init(void);
void OTA_system_loop(void);
void OTA_report_hw_version(void);
void OTA_mission_start(char flag);
#endif
