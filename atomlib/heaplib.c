/**
  ******************************************************************************
  * @file        sys.c
  * @author      古么宁
  * @brief       规约系统层文件，本文件提供内存管理，方便在没有操作系统的环境下使用
  * @date        2019-04-10  
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 GoodMorning
  *
  ******************************************************************************
  */
/* Includes -----------------------------------------------------------------*/
#include <stdlib.h> // for  size_t,NULL
#include "heaplib.h"



// 自适应内存对齐宽度，32/64，暂不支持其他位数的处理器
#define ALIGN_WIDTH (sizeof(size_t))

// 掩码
#define ALIGN_MASK  (ALIGN_WIDTH - 1)

typedef struct blockinfo { ///< 记录内存块信息
	int nextidx ;	/*<< The next free block in the list. */
	int size   ;    /*<< The size of the free block. */
}
blockinfo_t;


/// 内存池申请，可能存在地址不对齐的情况
static char bufpool[BUF_POOL_SIZE] = {0};

/// 记录 8 字节地址对齐的内存池起始地址
static char * bufstart = NULL;

/// 记录 8 字节地址对齐的内存池最大索引值
static int pool_index_max = 0 ;

/// 内存池管理链表表头 
static struct blockinfo liststart = {0};

/// 内存申请互斥
static sys_mutex_t memmtx ;


/**
  * @author   古么宁
  * @brief    内存申请
  * @param    wantedsize : 所需要的大小
  * @return   成功返回可用指针，否则返回 NULL
*/
void  * sys_malloc(size_t wantedsize)
{
	char * returnbuf = NULL;
	struct blockinfo * prevblock ;
	struct blockinfo * thisblock ;
	struct blockinfo * newblock  ;

	if (pool_index_max == 0) {                        // 第一次运行，先进行初始化。
		long addrbase = (long)bufpool;                // 获取基础地址
		long addrend  = addrbase + BUF_POOL_SIZE;     // 获取内存池末尾地址
		bufstart = (addrbase & ALIGN_MASK) ? &bufpool[ALIGN_WIDTH - (addrbase & ALIGN_MASK)] : bufpool ; // 8 字节对齐基地址
		pool_index_max = (addrend & (~ALIGN_MASK)) - ((long)bufstart);// 字节对齐的末地址

		newblock = (struct blockinfo *)bufstart; // 字节对齐，此处可以放心进行转换
		newblock->nextidx = pool_index_max ;     // 内存未占用，即下一个空闲块指向末尾
		newblock->size    = pool_index_max ;     // 内存未占用，大小为可用内存大小

		sys_mutex_init(&memmtx); // 对互斥量进行初始化
	}

	sys_mutex_lock(&memmtx); //保证线程安全

	wantedsize += sizeof(struct blockinfo);

	if (wantedsize & ALIGN_MASK) {  // 统一按照 8 字节对齐处理
		wantedsize &= (~ALIGN_MASK);
		wantedsize += ALIGN_WIDTH ;
	}

	prevblock = &liststart ;
	
	while((prevblock->nextidx != pool_index_max) && (returnbuf == NULL) ) {
		char * thisfreebuf = &bufstart[prevblock->nextidx];
		thisblock = (struct blockinfo *)thisfreebuf;// 由于在处理上都是 8 字节对齐，此处可以放心进行转换

		if (thisblock->size >= wantedsize) { //当前块大小满足需求
			returnbuf = thisfreebuf + sizeof(struct blockinfo);
			if (thisblock->size - wantedsize > sizeof(struct blockinfo)) { // 扣去申请内存后所剩余的空间足够标记
				newblock = (struct blockinfo *)(thisfreebuf + wantedsize); // 产生一个新的空闲块
				newblock->size      = thisblock->size - wantedsize;        // 新的空闲块大小为原空闲块减去申请大小
				newblock->nextidx   = thisblock->nextidx ;                 // 新的空闲块链接指向下一个空闲块地址
				prevblock->nextidx += wantedsize;                          // 上一个空闲块链接到新的空闲块地址
				thisblock->size     = wantedsize;                          // 当前块已被占用，大小为所申请的大小
				thisblock->nextidx  = 0 ;                                  // 当前块已被占用
			}
			else { // 扣去申请内存后所剩余的空间不足够标记
				prevblock->nextidx = thisblock->nextidx ; // 上一个空闲块跳过当前块直接链接到下一个空闲块
				thisblock->nextidx = 0 ;                  // 当前块已被占用，
			}
		}
		else {//当前块不够内存申请，查找下一个块
			prevblock = thisblock; 
		}
	}

	sys_mutex_unlock(&memmtx);
	return returnbuf;
}

/**
  * @author   古么宁
  * @brief    内存释放
  * @param    buff : 所需要释放的内存
  * @return   don't care
*/
void sys_free(void * buff)
{
	struct blockinfo * prevblock ;
	struct blockinfo * thisblock ;
	char * thisbuf = buff ;
	char * nextbuf ;
	int    thisblockidx ; // 当前块所在的数组索引下标

	thisbuf -= sizeof(struct blockinfo); // 计算当前所释放内存的块信息位置
	thisblockidx = thisbuf - bufstart  ; // 获得当前块的下标索引

	if (thisblockidx < 0 || thisblockidx > pool_index_max) // 这块内存不归我管
		return  ;
	
	thisblock = (struct blockinfo *)thisbuf; // 获得当前块的信息

	sys_mutex_lock(&memmtx);      // 要读取索引信息，上锁以保证线程安全

	prevblock = &liststart ;
	nextbuf = &bufstart[prevblock->nextidx] ;

	while ( nextbuf < thisbuf ) {  // 查找到当前块在空闲链表里的升序位置
		prevblock = (struct blockinfo *)nextbuf;
		nextbuf = &bufstart[prevblock->nextidx] ;
	}

	thisblock->nextidx = prevblock->nextidx;// 当前块插入到前面块和后面块之间
	prevblock->nextidx = thisblockidx    ;  // 前面的空闲块链接到当前块

	// 如果存在后一块并且可以和后一块合并
	if ((nextbuf == thisbuf + thisblock->size)&&(thisblock->nextidx != pool_index_max)) {
		struct blockinfo * nextblock = (struct blockinfo *)nextbuf;
		thisblock->size   += nextblock->size  ; // 把后一块大小合并到当前块
		thisblock->nextidx = nextblock->nextidx;// 当前块连接到后一块的后一块
	}

	if ( ((char *)prevblock) + prevblock->size == thisbuf) { // 能与前内存合并
		prevblock->size   += thisblock->size   ; // 前一块合并当前块的大小
		prevblock->nextidx = thisblock->nextidx; // 前一块连接到当前块的 next
	}
	
	sys_mutex_unlock(&memmtx);
}



