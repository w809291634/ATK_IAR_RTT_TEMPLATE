/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         古么宁
  * @brief          serial_console file
                    串口控制台文件。
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

#include "tasklib.h"
#include "iap_hal.h"

#include "shell.h"
#include "vim.h"


#include "stm32f1xx_hal.h" //for SCB->VTOR
#include "stm32f1xx_serial.h"
#include "stm32f1xx_console.h"

//--------------------相关宏定义及结构体定义--------------------

#define SYSTEM_CONFIG_FILE 




const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   ";



static struct serial_iap
{
	uint32_t timestamp;
	uint32_t addr;
	uint32_t size;
	char *  rxbuf;
	uint16_t rxbufmax;
}
iap;


static ros_task_t iap_timeout_task;
static ros_task_t serial_console_task;

static struct shell_input f1shell;


serial_t * ttyconsole = NULL;   //
//------------------------------相关函数声明------------------------------




//------------------------------华丽的分割线------------------------------



/**
	* @brief iap_check_complete iap 升级超时判断任务
	* @param void
	* @return NULL
*/
static int iap_check_complete(void * arg)
{
	struct shell_input * shell;
	uint32_t filesize ;
	char ** databuf ; 
	uint16_t *datamax ;

	TASK_BEGIN();     //任务开始
	
	printk("loading");//开始接收时打印 loading 进度条
	
	task_cond_wait(OS_current_time - iap.timestamp > 1600) ;//超时 1.6 s 无数据包，表示接收结束
	
	iap_lock_flash();   //由于要写完最后一包数据才能上锁，所以上锁放在 iap_check_complete 中
	
	filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
	
	shell = (struct shell_input*)arg;
	shell->gets = cmdline_gets;       //恢复串口命令行模式

	serial_close(ttyconsole);   // 关闭设备
	f1s_free(ttyconsole->rxbuf);  // 释放内存
	databuf = (char**)(&ttyconsole->rxbuf) ;
	datamax = (uint16_t *)&ttyconsole->rxmax ;
	*databuf = iap.rxbuf ;
	*datamax = iap.rxbufmax;
	serial_open(ttyconsole,115200,8,'N',1);
	printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);
	TASK_END();
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
		shell_input(&f1shell,packet,pktlen);//数据帧传入应用层
	}
	
	TASK_END();
}



/** 
	* @brief iap_gets  
	*        iap 升级任务，获取数据流并写入flash
	*        注：由于在写 flash 的时候，单片机会停止读取 flash，即
	*        代码不会运行。如果在写 flash 的时候有中断产生，单片机
	*        可能会死机。写 flash 的时候不妨碍 dma 的传输，所以 dma
	*        接收缓冲要大一些，要在下一包数据接收完写完当前包数据
	* @param void
	* @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	uint32_t * value = (uint32_t*)buf;
	
	for (iap.size = iap.addr + len ; iap.addr < iap.size ; iap.addr += 4)
		iap_write_flash(iap.addr,*value++); 
	
	//printk("size=%d\r\n",len);
	iap.timestamp = OS_current_time;//更新时间戳
	
	if ((iap.addr & (FLASH_PAGE_SIZE-1)) == 0)//清空下一页
		iap_erase_flash(iap.addr ,1) ;//printk("erase 0x%x\r\n",iap.addr);//
//	else
	if (task_is_exited(&iap_timeout_task))//开始接收后创建超时任务，判断接收结束
		task_create(&iap_timeout_task,NULL,iap_check_complete,shell);
	else
		printl(".",1);//打印一个点以示清白
}


/**
	* @brief    shell_iap_command
	*           命令行响应函数
	* @param    arg  : 命令行内存指针
	* @return   void
*/
void shell_iap_command(void * arg)
{
	int argc , erasesize ;
	
	struct shell_input * shell = container_of(arg, struct shell_input, cmdline);
	
	if (shell != &f1shell) { //防止其他 shell 调用此命令，否则会擦除掉 flash
		printk("cannot update in this channal\r\n");
		return ;
	}

	char * iapdata = f1s_malloc(FLASH_PAGE_SIZE*2);
	if (!iapdata) {
		printk("cannot malloc buffer for iap\r\n");
		return ;
	}
	
	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
	shell->gets = iap_gets;//串口数据流获取至 iap_gets
	
	argc = cmdline_param((char*)arg,&erasesize,1);
	
	iap.addr = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;
	iap.size = (argc == 1) ? erasesize : 1 ;

	//由于要写完最后一包数据才能上锁，所以上锁放在 iap_check_complete 中
	iap_unlock_flash();
	iap_erase_flash(iap.addr , iap.size);

	tcdrain(ttyconsole);

	char ** databuf = (char **)&ttyconsole->rxbuf ;
	uint16_t *  datamax = (uint16_t *)&ttyconsole->rxmax ;
	serial_close(ttyconsole);       // 关闭设备，重新打开
	iap.rxbuf = ttyconsole->rxbuf ;   // 记录原有的 buf
	iap.rxbufmax = ttyconsole->rxmax ;// 记录原有的 max
	*databuf = iapdata;
	*datamax = FLASH_PAGE_SIZE ;
	serial_open(ttyconsole,115200,8,'N',1); // 重新打开
}






void shell_erase_flash(void * arg)
{
	static const char tips[] = "flash-erase (addr) [page-size]\r\n";
	int argc ;
	int argv[2];

	argc = cmdline_param((char*)arg,argv,2);

	printk("erase flash ");

	if (argc < 1) {
		printl((char*)tips,sizeof(tips)-1);
	}
	else {
		uint32_t addr = argv[0];
		uint32_t size = (argc == 1)? 1:argv[1];//默认擦除一个扇区
		
		if (iap_unlock_flash()) {
			color_printk(light_red,"error,cannot unlock flash\r\n");
			return ;
		}

		if (iap_erase_flash(addr,size)) {
			color_printk(light_red,"error,cannot erase flash\r\n");
			return ;
		}
		
		if (iap_lock_flash()) {
			color_printk(light_red,"error,cannot lock flash\r\n");
			return ;
		}
		
		printk("(0x%x)done\r\n",addr);
	}
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


#endif //#ifdef SYSTEM_CONFIG_FILE


/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void f1shell_puts(const char * buf,uint16_t len)
{
	serial_write(ttyconsole,buf,len,O_NOBLOCK);
}



/**
	* @brief    serial_console_init
	*           初始化串口控制台
	* @param
	* @return   void
*/
void serial_console_init(char * info)
{
	ttyconsole = &ttyS1 ; // 控制台输出设备

	serial_open(ttyconsole,115200,8,'N',1);
	shell_input_init(&f1shell,f1shell_puts);

	shell_register_command("reboot"     ,shell_reboot_command);
	shell_register_command("flash-erase",shell_erase_flash);
	
	if (ttyconsole->hal->dma_rx_enable) { // 当接收为 DMA 模式下，注册在线升级命令
		if (SCB->VTOR == FLASH_BASE)
			shell_register_command("update-app",shell_iap_command);	
		else
			shell_register_confirm("update-iap",shell_iap_command,"sure to update iap?");
	}

	#ifdef SYSTEM_CONFIG_FILE
		shell_register_command("syscfg",_shell_edit_syscfg);
	#endif
	
	#ifdef OS_USE_ID_AND_NAME
		shell_register_command("top",shell_show_protothread);
		shell_register_command("kill",shell_kill_protothread);
	#endif
	
	task_create(&serial_console_task,NULL,serial_console_recv,NULL);
	
	if (info) {
		f1shell_puts("\r\n",2);
		f1shell_puts(info,strlen(info));//打印开机信息或者控制台信息
		f1shell_puts("\r\n",2);
//		for (int i = 0 ; ttyconsole->txtail ; i++) ;
	}
}


void serial_console_deinit(void)
{
	serial_close(ttyconsole);
}


