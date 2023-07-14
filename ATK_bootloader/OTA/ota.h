#ifndef __OTA_H__
#define __OTA_H__
#include "sys.h"
#include "esp32_at.h"

void OTA_check_version(void);



int http_data_handle(char *buf,uint16_t len);
void OTA_system_init(void);
void OTA_system_loop(void);
#endif
