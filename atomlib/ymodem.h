/**
  ******************************************************************************
  * @file           
  * @author         GoodMorning
  * @brief          ymodem 实现接口对外头文件
  * @note
  * <pre>
  * 使用步骤：
  * 0> 编写用户文件操作接口 struct ymodem_fs fs; 实现所需的文件操作  
  *    编写硬件发送函数 int yputs(void * from,const void * data, int len);  
  *    硬件接收到的数据无论是否断包通过 ymodem_recv(&ymod,...) 传入;
  * 
  * 1> 新建传输 struct ymodem ymod ;
  * 
  * 2> 初始化 ymod 的接口 ymodem_init(&ymod,&fs,yputs);
  * 
  * 3> 对 ymod 进行每秒 1 次的 ymodem_period(&ymod) 调用 
  * 
  * 4> 调用 ymodem_start_recv(&ymod) 使能开始接收文件；
  * 
  * *> 如果是仅作接收，则只需要实现 open,close,write 接口即可
  * </pre>
  ******************************************************************************
  *
  * COPYRIGHT(c) 2020 GoodMorning
  *
  ******************************************************************************
*/
#ifndef _Y_MODEM_H
#define _Y_MODEM_H

/** 定义文件类型，可根据不同的应用定义为不同的类型，用于 struct ymodem_fs 文件接口 */
typedef int   yfile_t ;

/**@name    用户文件接口定义
  * @brief  错误时请返回负数
  * @{
*/
/** 根据文件名 filename 打开一个文件，句柄输出为 fd */
typedef int (*yfopen_t)(yfile_t * fd , const char * filename);

/** 关闭文件 fd */
typedef int (*yfclose_t)(yfile_t * fd);

/** 对文件 fd 写入 len 个字节的数据 data */ 
typedef int (*yfdata_t)(yfile_t * fd,void * data, int len);
/** @}*/

/** y modem 硬件发送函数类型 */
typedef int (*yputs_t)(void * from,const void * data, int len);


/* y-modem 文件接口 */
struct ymodem_fs {
	yfopen_t    open   ; ///< 打开文件接口
	yfclose_t   close  ; ///< 关闭文件接口
	yfdata_t    write  ; ///< 写入文件接口,对文件 fd 写入 len 个字节的数据 data
	yfdata_t    read   ; ///< 读取文件接口,从文件 fd 读取 len 个字节的数据至 data
};


/* y-modem 文件接口 */
struct ymodem {
	struct ymodem_fs  fs          ; ///< 文件操作接口
	yfile_t           fd          ; ///< 文件句柄
	yputs_t           puts        ; ///< 硬件输出函数接口
	int               time        ; ///< 超时计数
	int               fsize       ; ///< size of file
	int               recv        ; ///< 已接收的文件大小
	int               stat        ; ///< 传输状态机
	int               tail        ; ///< tail of buffer
	unsigned char     nums        ; ///< number of received packets
	char              buf[1024+7] ; ///< buffer
};



/**
  * @author   GoodMorning
  * @brief    initialize a ymodem device
  * @param    ymodem   : ymodem module
  * @param    fs       : user file operation interface
  * @param    puts     : 
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_init(
	struct ymodem *    ymodem ,
	struct ymodem_fs * fs     ,
	yputs_t            puts   ) ;


/**
  * @author   GoodMorning
  * @brief    ymodem device period hander
  * @param    ymodem    : ymodem module
  * @param    timestamp : user timestamp
  * @return   don't care
*/
void ymodem_period(struct ymodem * ymodem) ;



/**
  * @author   GoodMorning
  * @brief    ymodem device start to receive
  * @param    ymodem   : ymodem module
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_start_recv(struct ymodem *ymod) ;


/**
  * @author   GoodMorning
  * @brief    ymodem device start to receive
  * @param    ymodem   : ymodem module
  * @return   On seccess , 0 is returned;On error, -1 is returned.
*/
int ymodem_recv(struct ymodem * ymod , void * data , int len);

#endif /* _Y_MODEM_H */
