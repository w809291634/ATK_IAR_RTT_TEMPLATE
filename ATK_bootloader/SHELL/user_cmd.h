#ifndef __USER_CMD_H__
#define __USER_CMD_H__
#include "sys.h"

#define  ESP_SET_SSID_PASS_CMD               "esp_set_wifi"
#define  ESP_GET_SSID_PASS_CMD               "esp_get"
#define  IAP_CMD                             "iap"
#define  APP_START                           "enter_app"

void register_user_cmd(void);
#endif
