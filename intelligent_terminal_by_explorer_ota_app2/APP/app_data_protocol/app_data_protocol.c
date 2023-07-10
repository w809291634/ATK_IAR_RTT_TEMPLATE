#include "app_data_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char wbuf[256];

static int (*SysProcess_command_callback)(char* , char* , char* );
static int (*UserProcess_command_callback)(char* , char* , char* );

void zxbeeBegin(void)
{
  wbuf[0] = '{';
  wbuf[1] = '\0';
}

int zxbeeAdd(char* tag, char* val)
{
  int offset = strlen(wbuf);
  sprintf(&wbuf[offset],"%s=%s,", tag, val);
  return 0;
}

char* zxbeeEnd(void)
{
  int offset = strlen(wbuf);
  wbuf[offset-1] = '}';
  wbuf[offset] = '\0';
  if (offset > 2) return wbuf;
  return NULL;
}

//pkg 数据包
void zxbee_onrecv_fun(char *pkg)
{
  int len = strlen(pkg);
  char* p=pkg;
  char* pcmd;
  char* pdata;
  char* _wbuf=0;

  if(pkg[0]!='{' || pkg[len-1]!='}') return;
  pkg[len-1]=0;
  p++;
  do{
    pcmd=p;           //取数据头
    p=strchr(p,'=');  //指向'='
    *p++=0;
    pdata=p;
    if(p!=NULL){
      pdata=p;
      p=strchr(p,',');
      if(p!=NULL)*p++=0;

      //pcmd:指令 pdata:数据
      {
        int ret;
        ret = SysProcess_command_callback(pcmd, pdata, _wbuf);
        if (ret < 0)
        {
          ret = UserProcess_command_callback(pcmd, pdata, _wbuf);
        }
      }
    }
  }while (p!=NULL);
}

void zxbee_onrecv_call_fun_set(int (*Sys_callback)(char* , char* , char* ),int (*User_callback)(char* , char* , char* ))
{
  SysProcess_command_callback=Sys_callback;
  UserProcess_command_callback=User_callback;
}
