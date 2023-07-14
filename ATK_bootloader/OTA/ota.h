#ifndef __OTA_H__
#define __OTA_H__
#include "sys.h"
#include "esp32_at.h"

int http_data_handle(char *buf,uint16_t len);
void OTA_system_init(void);
void OTA_system_loop(void);
void OTA_report_hw_version(void);
void OTA_update_start(void);
#endif
