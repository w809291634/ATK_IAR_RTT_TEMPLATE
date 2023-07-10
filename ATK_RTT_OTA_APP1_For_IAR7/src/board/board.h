/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      SummerGift   first version
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include <stm32f4xx.h>
#include "drv_gpio.h"
#include <rthw.h>
#include <rtdevice.h>
#include "iap_config.h"

#ifdef __cplusplus
extern "C" {
#endif
 
void _Error_Handler(char *s, int num);
void SystemClock_Config(void);
void NVIC_SetVectorTable(uint32_t NVIC_VectTab, uint32_t Offset);

#ifndef Error_Handler
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)
#endif

#define DMA_NOT_AVAILABLE ((DMA_INSTANCE_TYPE *)0xFFFFFFFFU)
#define CCMRAM __attribute__((section(".sram")))         // 使用CCM，IAR8支持

#define NVIC_VectTab_RAM             ((uint32_t)0x20000000)
#define NVIC_VectTab_FLASH           ((uint32_t)0x08000000)
#define IS_NVIC_VECTTAB(VECTTAB) (((VECTTAB) == NVIC_VectTab_RAM) || \
                                  ((VECTTAB) == NVIC_VectTab_FLASH))

#define STM32_FLASH_START_ADRESS     ((uint32_t)0x08000000)
#define STM32_FLASH_SIZE_BYTE        (1024 * 1024)
#define STM32_FLASH_END_ADDRESS      ((uint32_t)(STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE_BYTE))

// 由于 CSTACK 分配原因，这里SRAM1不支持自动分配
//#define STM32_SRAM1_SIZE            (128)
//#define STM32_SRAM1_END             (0x20000000 + STM32_SRAM1_SIZE * 1024)
//#define STM32_SRAM_SIZE             STM32_SRAM1_SIZE
//#define STM32_SRAM_END              STM32_SRAM1_END

// CCM RAM 使用自动分配剩余内存
#ifdef RT_CCMRAM_AUTO_ALLOC
#define STM32_SRAM2_CCM_SIZE        (64)
#define STM32_SRAM2_CCM_END         (0x10000000 + STM32_SRAM2_CCM_SIZE * 1024)
#define STM32_SRAM_SIZE             STM32_SRAM2_CCM_SIZE
#define STM32_SRAM_END              STM32_SRAM2_CCM_END

#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN      ((void *)&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
// iar使用
#pragma section="CSTACK"
#define HEAP_BEGIN      (__segment_end("CSTACK"))
#else
extern int __bss_end;
#define HEAP_BEGIN      ((void *)&__bss_end)
#endif

#define HEAP_END        STM32_SRAM_END
#endif    // RT_CCMRAM_AUTO_ALLOC

#ifdef __cplusplus
}
#endif

#endif

