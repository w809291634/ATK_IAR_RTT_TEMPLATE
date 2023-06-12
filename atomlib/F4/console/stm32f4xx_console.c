/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         古么宁
  * @brief          serial_console file
                    串口控制台文件。文件不直接操作硬件，依赖 serial_hal.c 和 iap_hal.c
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //定义了很多数据类型

#include "stm32f429xx.h" //for SCB->VTOR
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#include "iap_hal.h"

#include "stm32f4xx_serial.h"
#include "stm32f4xx_console.h"

#include "shell.h"
#include "tasklib.h"

#include "ymodem.h"


/* Private macro ------------------------------------------------------------*/

#define SYSTEM_CONFIG_FILE 

#ifdef SYSTEM_CONFIG_FILE
#	include "vim.h"
#endif

/* Private variables ------------------------------------------------------------*/

const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   please send file by Y-modem:";


//static const char division [] = "\r\n----------------------------\r\n";

#if (SYS_OS_USE)

#include "cmsis_os.h" // 启用 freertos
osThreadId SerialConsoleTaskHandle;

#else 

static ros_task_t iap_timeout_task;
static ros_task_t serial_console_task;

#endif

static struct shell_input f4shell;
static struct ymodem      ymod   ;
/* Global variables ------------------------------------------------------------*/

serial_t * ttyconsole = NULL;

/* Private function prototypes -----------------------------------------------*/
void f4shell_puts(const char * buf,uint16_t len) ; 


/* Gorgeous Split-line 华丽的分割线------------------------------------*/


/**
  * @brief iap_check_complete iap 升级超时判断任务
  * @param void
  * @return NULL
*/
static int iap_check_complete(void * arg)
{
	TASK_BEGIN();//任务开始

	while(ymod.stat) {
		ymodem_period(&ymod);
		task_sleep(1000);
	}

	printk("\r\nrecv binary complete,size:%d\r\n",ymod.fsize);
	printk("reset console.\r\n");
	f4shell.gets = cmdline_gets ;

	TASK_END();
}



static void flash_erase(uint32_t flash_addr)
{
	#define SIZE (sizeof(f4xx_sectors) / sizeof(f4xx_sectors[0]))

	uint32_t SectorError;

	for (int i = 0; i < SIZE ; i++) {
		if (f4xx_sectors[i] == flash_addr) {
			FLASH_EraseInitTypeDef erase;
			erase.TypeErase    = FLASH_TYPEERASE_SECTORS; //擦除类型，扇区擦除 
			erase.Sector       = i                      ; //扇区
			erase.NbSectors    = 1                      ; //一次只擦除一个扇区
			erase.VoltageRange = FLASH_VOLTAGE_RANGE_3  ; //电压范围，VCC=2.7~3.6V之间!!
			HAL_FLASHEx_Erase(&erase,&SectorError);
			return ;
		}
		else
		if (f4xx_sectors[i] > flash_addr){
			return ;
		}
	}
}


/** 
  * @brief iap 升级任务，获取数据流并写入flash
  * @param void
  * @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , int len)
{
	ymodem_recv(&ymod,buf,len);
}



static int yfopen(yfile_t * fd , const char * filename)
{
	*fd = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;
	HAL_FLASH_Unlock();
	return 0;
}

static int yfclose(yfile_t * fd)
{
	HAL_FLASH_Lock();
	return 0;
}

static int yfwrite(yfile_t * fd,void * data, int len)
{
	uint32_t value;
	uint8_t * buf = (uint8_t *)data;
	len /= 4 ;
	for (int i = 0; i < len ; i++) {
		value = buf[0] | (buf[1] << 8) | (buf[2] << 16)|(buf[3] << 24);
		if (0 == (*fd & (0x4000-1))) {
			flash_erase(*fd);
		}
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,*fd,value);
		(*fd) += 4 ;
		buf   += 4 ;
	}
	return 0;
}

/**
  * @brief    shell_iap_command
  *           命令行响应函数
  * @param    arg  : 命令行内存指针
  * @return   void
*/
void shell_iap_command(void * arg)
{
	//int argc , erasesize ;
	struct shell_input * shell = container_of(arg, struct shell_input, cmdline);
	
	if (shell != &f4shell) { //防止其他 shell 调用此命令，否则会擦除掉 flash
		printk("cannot update in this channal\r\n");
		return ;
	}

	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
	shell->gets = iap_gets  ;//串口数据流获取至 iap_gets

	ymodem_start_recv(&ymod);
	task_create(&iap_timeout_task,NULL,iap_check_complete,shell);
}





/** 
  * @brief serial_console_recv  串口控制台处理任务，不直接调用
  * @param void
  * @return int 
*/
static int serial_console_recv(void * arg)
{
	char  *  packet;
	uint16_t pktlen ;

	TASK_BEGIN();//任务开始

	while(1) {
		task_cond_wait(pktlen = serial_gets(ttyconsole,&packet,O_NOBLOCK));//等待串口接收
		shell_input(&f4shell,packet,pktlen);//数据帧传入应用层
	}
	
	TASK_END();
}





#ifdef OS_USE_ID_AND_NAME
/**
  * @brief    shell_show_protothread
  *           命令行响应函数,列出所有 protothread
  * @param
  * @return   void
*/
void shell_show_protothread(void * arg)
{
	struct list_head * TaskListNode;
	struct protothread * pthread;

	if (list_empty(&OS_scheduler_list)) 
		return;

	printk("\r\n\tID\t\tThread\r\n");

	list_for_each(TaskListNode,&OS_scheduler_list) {
		pthread = list_entry(TaskListNode,struct protothread,list_node);
		printk("\t%d\t\t%s\r\n",pthread->ID, pthread->name);
	}
}


/**
  * @brief    shell_kill_protothread
  *           命令行响应函数,删除某个 protothread
  * @param
  * @return   void
*/
void shell_kill_protothread(void * arg)
{
	int searchID ;

	struct list_head * search_list;
	struct protothread * pthread;

	if (1 != cmdline_param((char*)arg,&searchID,1))
		return ;

	list_for_each(search_list, &OS_scheduler_list) {
		pthread = list_entry(search_list,struct protothread,list_node);
		if (searchID == pthread->ID) {
			task_cancel(pthread);
			printk("\r\nKill %s\r\n",pthread->name);
		}
	}
}

#endif


#ifdef SYSTEM_CONFIG_FILE

/**
  * @brief    _syscfg_fgets
  *          获取 syscfg 信息，由 shell_into_edit 调用
  * @param
  * @return   成功 返回VIM_FILE_OK
*/
uint32_t _syscfg_fgets(char * fpath, char * fdata,uint16_t * fsize)
{
	uint32_t len = 0;
	uint32_t addr = syscfg_addr();
	
	if (0 == addr)
		return VIM_FILE_ERROR;

	for (char * syscfg = (char*)addr ; *syscfg ; ++len)
		*fdata++ = *syscfg++ ;

	*fsize = len;

	return VIM_FILE_OK;
}

/**
  * @brief    _syscfg_fputs
  *          更新 syscfg 信息，由 shell_into_edit 调用
  * @param
  * @return   void
*/
void _syscfg_fputs(char * fpath, char * fdata,uint32_t fsize)
{
	syscfg_write(fdata,fsize);
}



/**
  * @brief    _shell_edit_syscfg
  *           命令行编辑 syscfg 信息
  * @param    arg  命令行内存
  * @return   void
*/
void _shell_edit_syscfg(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	shell_into_edit(shellin , _syscfg_fgets , _syscfg_fputs );
}


/**
  * @brief    _shell_rm_syscfg
  *           命令行删除 syscfg 信息
  * @param    arg  命令行内存
  * @return   void
*/
void _shell_rm_syscfg(void * arg)
{
	syscfg_erase();
}

#endif //#ifdef SYSTEM_CONFIG_FILE


/**
  * @brief    hal_usart_puts console 硬件层输出
  * @param    空
  * @return   空
*/
void f4shell_puts(const char * buf,uint16_t len)
{
	serial_write(ttyconsole,buf,len,O_BLOCKING);
}

int shell_yputs(void * from ,const void * data , int len)
{
	return serial_write(ttyconsole,data,len,O_BLOCKING);
}


/**
  * @brief    vSystemReboot 硬件重启
  * @param    空
  * @return
*/
void shell_reboot_command(void * arg)
{
	NVIC_SystemReset();
}



/**
  * @brief    serial_console_init
  *           串口控制台初始化
  * @param    info:  初始化信息
  * @return   void
*/
void serial_console_init(char * info)
{	
	struct ymodem_fs fs = {0};

	ttyconsole = &ttyS1 ; // 控制台输出设备

	serial_open(ttyconsole,115200,8,'N',1);

	SHELL_INPUT_INIT(&f4shell,f4shell_puts);//新建交互，输出为串口输出

	shell_register_command("reboot"  ,shell_reboot_command);

	//根据 SCB->VTOR 判断当前所运行的代码为 app 还是 iap.不同区域注册不一样的命令
	if (SCB->VTOR == FLASH_BASE)
		shell_register_command("update-app",shell_iap_command);
	else
		shell_register_confirm("update-iap",shell_iap_command,"sure to update iap?");
	
	#ifdef SYSTEM_CONFIG_FILE
		shell_register_command("syscfg",_shell_edit_syscfg);
	#endif

	#ifdef OS_USE_ID_AND_NAME
		shell_register_command("top",shell_show_protothread);
		shell_register_command("kill",shell_kill_protothread);
	#endif

	//创建一个串口接收任务，串口接收到一包传入 serial_console_recv 
	task_create(&serial_console_task,NULL,serial_console_recv,NULL);
	
	fs.write = yfwrite ;
	fs.open  = yfopen  ;
	fs.close = yfclose ;
	ymodem_init(&ymod,&fs,shell_yputs);

	if (info) {
		f4shell_puts("\r\n",2);
		f4shell_puts(info,strlen(info));//打印开机信息或者控制台信息
		f4shell_puts("\r\n",2);

		for (int i = 0 ; ttyconsole->txtail ; i++) ;
	}
}



void serial_console_deinit(void)
{
	serial_close(ttyconsole);
}


