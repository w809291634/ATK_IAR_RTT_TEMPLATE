/**
  ******************************************************************************
  * @file           heaplib.h
  * @author         古么宁
  * @brief          内存管理头文件，方便在没有操作系统的时候进行内存管理
  *                 一般情况下操作系统都会带内存管理，此处提供无操作系统下的内存管理，
  *                 但在接口的实现上依然保留操作系统支持。
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
*/
#ifndef HEAP_LIB_H__
#define HEAP_LIB_H__


#define BUF_POOL_SIZE (8*1024)  ///< 内存池大小

#define ENABLE_MULTITHREAD   0  ///< 多线程支持，需要提供线程互斥量实现

#if (ENABLE_MULTITHREAD == 0) // 如果无操作系统，互斥锁相关宏定义可以留空

	typedef int sys_mutex_t ;
	#define sys_mutex_lock(x)
	#define sys_mutex_unlock(x)
	#define sys_mutex_init(x)

#else   // 需要多线程支持则需要提供互斥量。

	#include <pthread.h>

	// 定义互斥锁类型
	typedef pthread_mutex_t sys_mutex_t ;

	// 定义基本的互斥锁操作
	#define sys_mutex_lock(x)   pthread_mutex_lock((x))
	#define sys_mutex_unlock(x) pthread_mutex_unlock((x))
	#define sys_mutex_init(x)   pthread_mutex_init((x), NULL)

#endif



extern void * sys_malloc(size_t wantedsize) ;
extern void   sys_free  (void * buff) ;


#endif

