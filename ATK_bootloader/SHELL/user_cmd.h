#ifndef __USER_CMD_H__
#define __USER_CMD_H__
#include "sys.h"

#define  ESP_SET_SSID_PASS_CMD               "esp_set_id_pd"
#define  ESP_GET_SSID_PASS_CMD               "esp_get_id_pd"

void register_user_cmd(void);
#endif
