/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */
#include "board.h"
#include "main.h"

/** 用户编写部分 **/

#ifdef RT_HEAP_USING_CCMRAM
#ifndef RT_CCMRAM_AUTO_ALLOC

#define RT_HEAP_SIZE 7680                           // use: RT_HEAP_SIZE*4 (Byte) 30KB
#pragma default_variable_attributes = @ ".sram"
static uint32_t rt_heap[RT_HEAP_SIZE];              // 使用CCM的RAM空间
#pragma default_variable_attributes =
RT_WEAK void *rt_heap_begin_get(void)
{
  return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
  return rt_heap + RT_HEAP_SIZE;
}
#endif  //RT_CCMRAM_AUTO_ALLOC

#else // 不使用CCM。使用0x2**的RAM
#define RT_HEAP_SIZE 7680                           // use: RT_HEAP_SIZE*4 (Byte)
static uint32_t rt_heap[RT_HEAP_SIZE];  
RT_WEAK void *rt_heap_begin_get(void)
{
  return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
  return rt_heap + RT_HEAP_SIZE;
} 
#endif  //RT_HEAP_USING_CCMRAM 

/* SystemClock_Config */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /**Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

///*************微秒延时函数*************/
//us不要超过32位数值的范围
//该函数时候时必须在调度器启动之后使用，否则systick没有启动，导致一直在这里循环
void hw_us_delay(unsigned int us)
{
    unsigned int start=0, now=0, reload=0, us_tick=0 ,tcnt=0 ;
    start = SysTick->VAL;                       //systick的当前计数值（起始值）
    reload = SysTick->LOAD;                     //systick的重载值
    us_tick = SystemCoreClock / 1000000UL;   //1us下的systick计数值，SystemCoreClock为1s下的计数值，需要配置systick时钟源为HCLK
    do {
      now = SysTick->VAL;                       //systick的当前计数值
      if(now!=start){
        tcnt += now < start ? start - now : reload - now + start;    //获取当前systick经过的tick数量，统一累加到tcnt中
        start=now;                              //重新获取systick的当前计数值（起始值）
      }
    } while(tcnt < us_tick * us);               //如果超出，延时完成
}

// 设置向量表
void NVIC_SetVectorTable(uint32_t NVIC_VectTab, uint32_t Offset)
{ 
  /* Check the parameters */
  assert_param(IS_NVIC_VECTTAB(NVIC_VectTab));
  assert_param(IS_NVIC_OFFSET(Offset));  
   
  SCB->VTOR = NVIC_VectTab | (Offset & (uint32_t)0x1FFFFF80);
}

/** 拷贝drv_common.c部分 **/
#ifdef RT_USING_SERIAL
#include "drv_usart.h"
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char **argv)
{
    rt_hw_cpu_reset();
}
FINSH_FUNCTION_EXPORT_ALIAS(reboot, __cmd_reboot, Reboot System);
#endif /* RT_USING_FINSH */

/* SysTick configuration */
void rt_hw_systick_init(void)
{
#if defined (SOC_SERIES_STM32H7)
    HAL_SYSTICK_Config((HAL_RCCEx_GetD1SysClockFreq()) / RT_TICK_PER_SECOND);
#elif defined (SOC_SERIES_STM32MP1)
    HAL_SYSTICK_Config(HAL_RCC_GetSystemCoreClockFreq() / RT_TICK_PER_SECOND);
#else
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / RT_TICK_PER_SECOND);
#endif
#if !defined (SOC_SERIES_STM32MP1)
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
#endif
    NVIC_SetPriority(SysTick_IRQn, 0xFF);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_IncTick();
    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

uint32_t HAL_GetTick(void)
{
    return rt_tick_get_millisecond();
}

void HAL_SuspendTick(void)
{
}

void HAL_ResumeTick(void)
{
}

void HAL_Delay(__IO uint32_t Delay)
{
    if (rt_thread_self())
    {
        rt_thread_mdelay(Delay);
    }
    else
    {
        for (rt_uint32_t count = 0; count < Delay; count++)
        {
            rt_hw_us_delay(1000);
        }
    }
}

/* re-implement tick interface for STM32 HAL */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char *s, int num)
{
    /* USER CODE BEGIN Error_Handler */
    /* User can add his own implementation to report the HAL error return state */
    while (1)
    {
    }
    /* USER CODE END Error_Handler */
}

/**
 * This function will delay for some us.
 *
 * @param us the delay time of us
 */
void rt_hw_us_delay(rt_uint32_t us)
{
    rt_uint32_t start, now, delta, reload, us_tick;
    start = SysTick->VAL;
    reload = SysTick->LOAD;
    us_tick = SystemCoreClock / 1000000UL;
    do
    {
        now = SysTick->VAL;
        delta = start >= now ? start - now : reload + start - now;
    }
    while (delta < us_tick * us);
}

/**
 * This function will initial STM32 board.
 */
RT_WEAK void rt_hw_board_init()
{
#ifdef SCB_EnableICache
    /* Enable I-Cache---------------------------------------------------------*/
    SCB_EnableICache();
#endif

#ifdef SCB_EnableDCache
    /* Enable D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();
#endif
    /* HAL_Init() function is called at the beginning of the program */
    HAL_Init();
    
    system_partition_start();
    
    /* enable interrupt */
    __set_PRIMASK(0);
    /* System clock initialization */
    SystemClock_Config();
    /* disable interrupt */
    __set_PRIMASK(1);

    rt_hw_systick_init();

    /* Heap initialization */
#if defined(RT_USING_HEAP)  
#if defined(RT_CCMRAM_AUTO_ALLOC)
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#else
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif  // RT_CCMRAM_AUTO_ALLOC
#endif  // RT_USING_HEAP
    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    rt_hw_pin_init();
#endif

    /* USART driver initialization is open by default */
#ifdef RT_USING_SERIAL
    rt_hw_usart_init();
#endif

    /* Set the shell console output device */
#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* Board underlying hardware initialization */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

