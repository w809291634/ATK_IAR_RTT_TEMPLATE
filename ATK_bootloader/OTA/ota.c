#include "ota.h"
#include "usart3.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys.h"
#include "user_cmd.h"

#define AUTHORIZATION "version=2022-05-01&res=userid%2F315714&et=1720619752&method=sha1&sign=GG%2FUnpuqy6GC4L%2B45enSav3y3jA%3D" 
#define HOST          "iot-api.heclouds.com"


void http_request(int type,char *url)
{

}