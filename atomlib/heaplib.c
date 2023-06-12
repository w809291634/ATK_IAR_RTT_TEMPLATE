/**
  ******************************************************************************
  * @file        sys.c
  * @author      ��ô��
  * @brief       ��Լϵͳ���ļ������ļ��ṩ�ڴ����������û�в���ϵͳ�Ļ�����ʹ��
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



// ����Ӧ�ڴ�����ȣ�32/64���ݲ�֧������λ���Ĵ�����
#define ALIGN_WIDTH (sizeof(size_t))

// ����
#define ALIGN_MASK  (ALIGN_WIDTH - 1)

typedef struct blockinfo { ///< ��¼�ڴ����Ϣ
	int nextidx ;	/*<< The next free block in the list. */
	int size   ;    /*<< The size of the free block. */
}
blockinfo_t;


/// �ڴ�����룬���ܴ��ڵ�ַ����������
static char bufpool[BUF_POOL_SIZE] = {0};

/// ��¼ 8 �ֽڵ�ַ������ڴ����ʼ��ַ
static char * bufstart = NULL;

/// ��¼ 8 �ֽڵ�ַ������ڴ���������ֵ
static int pool_index_max = 0 ;

/// �ڴ�ع��������ͷ 
static struct blockinfo liststart = {0};

/// �ڴ����뻥��
static sys_mutex_t memmtx ;


/**
  * @author   ��ô��
  * @brief    �ڴ�����
  * @param    wantedsize : ����Ҫ�Ĵ�С
  * @return   �ɹ����ؿ���ָ�룬���򷵻� NULL
*/
void  * sys_malloc(size_t wantedsize)
{
	char * returnbuf = NULL;
	struct blockinfo * prevblock ;
	struct blockinfo * thisblock ;
	struct blockinfo * newblock  ;

	if (pool_index_max == 0) {                        // ��һ�����У��Ƚ��г�ʼ����
		long addrbase = (long)bufpool;                // ��ȡ������ַ
		long addrend  = addrbase + BUF_POOL_SIZE;     // ��ȡ�ڴ��ĩβ��ַ
		bufstart = (addrbase & ALIGN_MASK) ? &bufpool[ALIGN_WIDTH - (addrbase & ALIGN_MASK)] : bufpool ; // 8 �ֽڶ������ַ
		pool_index_max = (addrend & (~ALIGN_MASK)) - ((long)bufstart);// �ֽڶ����ĩ��ַ

		newblock = (struct blockinfo *)bufstart; // �ֽڶ��룬�˴����Է��Ľ���ת��
		newblock->nextidx = pool_index_max ;     // �ڴ�δռ�ã�����һ�����п�ָ��ĩβ
		newblock->size    = pool_index_max ;     // �ڴ�δռ�ã���СΪ�����ڴ��С

		sys_mutex_init(&memmtx); // �Ի��������г�ʼ��
	}

	sys_mutex_lock(&memmtx); //��֤�̰߳�ȫ

	wantedsize += sizeof(struct blockinfo);

	if (wantedsize & ALIGN_MASK) {  // ͳһ���� 8 �ֽڶ��봦��
		wantedsize &= (~ALIGN_MASK);
		wantedsize += ALIGN_WIDTH ;
	}

	prevblock = &liststart ;
	
	while((prevblock->nextidx != pool_index_max) && (returnbuf == NULL) ) {
		char * thisfreebuf = &bufstart[prevblock->nextidx];
		thisblock = (struct blockinfo *)thisfreebuf;// �����ڴ����϶��� 8 �ֽڶ��룬�˴����Է��Ľ���ת��

		if (thisblock->size >= wantedsize) { //��ǰ���С��������
			returnbuf = thisfreebuf + sizeof(struct blockinfo);
			if (thisblock->size - wantedsize > sizeof(struct blockinfo)) { // ��ȥ�����ڴ����ʣ��Ŀռ��㹻���
				newblock = (struct blockinfo *)(thisfreebuf + wantedsize); // ����һ���µĿ��п�
				newblock->size      = thisblock->size - wantedsize;        // �µĿ��п��СΪԭ���п��ȥ�����С
				newblock->nextidx   = thisblock->nextidx ;                 // �µĿ��п�����ָ����һ�����п��ַ
				prevblock->nextidx += wantedsize;                          // ��һ�����п����ӵ��µĿ��п��ַ
				thisblock->size     = wantedsize;                          // ��ǰ���ѱ�ռ�ã���СΪ������Ĵ�С
				thisblock->nextidx  = 0 ;                                  // ��ǰ���ѱ�ռ��
			}
			else { // ��ȥ�����ڴ����ʣ��Ŀռ䲻�㹻���
				prevblock->nextidx = thisblock->nextidx ; // ��һ�����п�������ǰ��ֱ�����ӵ���һ�����п�
				thisblock->nextidx = 0 ;                  // ��ǰ���ѱ�ռ�ã�
			}
		}
		else {//��ǰ�鲻���ڴ����룬������һ����
			prevblock = thisblock; 
		}
	}

	sys_mutex_unlock(&memmtx);
	return returnbuf;
}

/**
  * @author   ��ô��
  * @brief    �ڴ��ͷ�
  * @param    buff : ����Ҫ�ͷŵ��ڴ�
  * @return   don't care
*/
void sys_free(void * buff)
{
	struct blockinfo * prevblock ;
	struct blockinfo * thisblock ;
	char * thisbuf = buff ;
	char * nextbuf ;
	int    thisblockidx ; // ��ǰ�����ڵ����������±�

	thisbuf -= sizeof(struct blockinfo); // ���㵱ǰ���ͷ��ڴ�Ŀ���Ϣλ��
	thisblockidx = thisbuf - bufstart  ; // ��õ�ǰ����±�����

	if (thisblockidx < 0 || thisblockidx > pool_index_max) // ����ڴ治���ҹ�
		return  ;
	
	thisblock = (struct blockinfo *)thisbuf; // ��õ�ǰ�����Ϣ

	sys_mutex_lock(&memmtx);      // Ҫ��ȡ������Ϣ�������Ա�֤�̰߳�ȫ

	prevblock = &liststart ;
	nextbuf = &bufstart[prevblock->nextidx] ;

	while ( nextbuf < thisbuf ) {  // ���ҵ���ǰ���ڿ��������������λ��
		prevblock = (struct blockinfo *)nextbuf;
		nextbuf = &bufstart[prevblock->nextidx] ;
	}

	thisblock->nextidx = prevblock->nextidx;// ��ǰ����뵽ǰ���ͺ����֮��
	prevblock->nextidx = thisblockidx    ;  // ǰ��Ŀ��п����ӵ���ǰ��

	// ������ں�һ�鲢�ҿ��Ժͺ�һ��ϲ�
	if ((nextbuf == thisbuf + thisblock->size)&&(thisblock->nextidx != pool_index_max)) {
		struct blockinfo * nextblock = (struct blockinfo *)nextbuf;
		thisblock->size   += nextblock->size  ; // �Ѻ�һ���С�ϲ�����ǰ��
		thisblock->nextidx = nextblock->nextidx;// ��ǰ�����ӵ���һ��ĺ�һ��
	}

	if ( ((char *)prevblock) + prevblock->size == thisbuf) { // ����ǰ�ڴ�ϲ�
		prevblock->size   += thisblock->size   ; // ǰһ��ϲ���ǰ��Ĵ�С
		prevblock->nextidx = thisblock->nextidx; // ǰһ�����ӵ���ǰ��� next
	}
	
	sys_mutex_unlock(&memmtx);
}



