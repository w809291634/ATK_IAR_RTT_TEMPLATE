#ifndef __APP_DATA_PROTOCOL__
#define __APP_DATA_PROTOCOL__


#ifdef __cplusplus
extern "C" {
#endif

void zxbeeBegin(void);
int zxbeeAdd(char* tag, char* val);
char* zxbeeEnd(void);
void zxbee_onrecv_fun(char *pkg);
void zxbee_onrecv_call_fun_set(int (*Sys_callback)(char* , char* , char* ),int (*User_callback)(char* , char* , char* ));
#ifdef __cplusplus
}
#endif

#endif
