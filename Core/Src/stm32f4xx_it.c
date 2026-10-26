/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_util.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart1;

/*
 * 故障统一处理：
 * 先通过调试串口留标记，再主动复位。
 * 这样“程序卡死”会变成一次可恢复的重启，而不是永久停在 while(1)。
 */
static void Fault_Reset(const char *message, uint16_t length, uint32_t fault_code)
{
    volatile AppFaultRecord *record = App_GetFaultRecord();

    /* 先记录故障类型，再打印/复位；即使串口没接，下次启动也能看到 */
    record->code = fault_code;
    record->code_inv = ~fault_code;
    record->magic = APP_FAULT_RECORD_MAGIC;
    __DSB();

    if ((message != NULL) && (length > 0U))
    {
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)message, length, 50U);
    }

    __DSB();
    NVIC_SystemReset();

    while (1)
    {
    }
}
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
    * @brief This function handles Non maskable interrupt.
    */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
    static const char msg[] = "\r\n[NMI]\r\n";
    Fault_Reset(msg, (uint16_t)(sizeof(msg) - 1U), APP_FAULT_NMI);
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1)
    {
    }
    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
    * @brief This function handles Hard fault interrupt.
    */
void HardFault_Handler(void)
{
    /* USER CODE BEGIN HardFault_IRQn 0 */
    static const char hardfault_msg[] = "\r\n[HARDFAULT]\r\n";
    Fault_Reset(hardfault_msg, (uint16_t)(sizeof(hardfault_msg) - 1U), APP_FAULT_HARDFAULT);
    /* USER CODE END HardFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
    * @brief This function handles Memory management fault.
    */
void MemManage_Handler(void)
{
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */
    static const char msg[] = "\r\n[MEMFAULT]\r\n";
    Fault_Reset(msg, (uint16_t)(sizeof(msg) - 1U), APP_FAULT_MEMMANAGE);
    while (1)
    {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
    * @brief This function handles Pre-fetch fault, memory access fault.
    */
void BusFault_Handler(void)
{
    /* USER CODE BEGIN BusFault_IRQn 0 */
    static const char msg[] = "\r\n[BUSFAULT]\r\n";
    Fault_Reset(msg, (uint16_t)(sizeof(msg) - 1U), APP_FAULT_BUSFAULT);
    while (1)
    {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
    * @brief This function handles Undefined instruction or illegal state.
    */
void UsageFault_Handler(void)
{
    /* USER CODE BEGIN UsageFault_IRQn 0 */
    static const char msg[] = "\r\n[USAGEFAULT]\r\n";
    Fault_Reset(msg, (uint16_t)(sizeof(msg) - 1U), APP_FAULT_USAGEFAULT);
    while (1)
    {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
    * @brief This function handles System service call via SWI instruction.
    */
void SVC_Handler(void)
{
    /* USER CODE BEGIN SVCall_IRQn 0 */

    /* USER CODE END SVCall_IRQn 0 */
    /* USER CODE BEGIN SVCall_IRQn 1 */

    /* USER CODE END SVCall_IRQn 1 */
}

/**
    * @brief This function handles Debug monitor.
    */
void DebugMon_Handler(void)
{
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
    * @brief This function handles Pendable request for system service.
    */
void PendSV_Handler(void)
{
    /* USER CODE BEGIN PendSV_IRQn 0 */

    /* USER CODE END PendSV_IRQn 0 */
    /* USER CODE BEGIN PendSV_IRQn 1 */

    /* USER CODE END PendSV_IRQn 1 */
}

/**
    * @brief This function handles System tick timer.
    */
void SysTick_Handler(void)
{
    /* USER CODE BEGIN SysTick_IRQn 0 */

    /* USER CODE END SysTick_IRQn 0 */
    HAL_IncTick();
    /* USER CODE BEGIN SysTick_IRQn 1 */

    /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
    * @brief This function handles EXTI line0 interrupt.
    */
void EXTI0_IRQHandler(void)
{
    /* USER CODE BEGIN EXTI0_IRQn 0 */

    /* USER CODE END EXTI0_IRQn 0 */
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    /* USER CODE BEGIN EXTI0_IRQn 1 */

    /* USER CODE END EXTI0_IRQn 1 */
}

/**
    * @brief This function handles EXTI line[9:5] interrupts.
    */
void EXTI9_5_IRQHandler(void)
{
    /* USER CODE BEGIN EXTI9_5_IRQn 0 */

    /* USER CODE END EXTI9_5_IRQn 0 */
    HAL_GPIO_EXTI_IRQHandler(KEYBOARD_IRQ_Pin);
    HAL_GPIO_EXTI_IRQHandler(FP_IRQ_Pin);
    /*
     * 旧版这里误写成了 Led2_Pin（PA7，输出脚）。
     * 真正的 EXTI7 来源是霍尔/门磁 PE7，必须显式清它的挂起位，
     * 否则会一直进中断（中断风暴），表现为整机卡死。
     *
     * 现在 PE7 已改为普通输入、EXTI7 在 MX_GPIO_Init 里关闭，
     * 业务层也不使用 APP_EVENT_DOOR_SENSE，因此这里不再处理 Hall，
     * 避免电机转动/门磁抖动时 EXTI9_5 风暴饿死 SysTick。
     */
    /* USER CODE BEGIN EXTI9_5_IRQn 1 */

    /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
    * @brief This function handles TIM2 global interrupt.
    */
void TIM2_IRQHandler(void)
{
    /* USER CODE BEGIN TIM2_IRQn 0 */

    /* USER CODE END TIM2_IRQn 0 */
    HAL_TIM_IRQHandler(&htim2);
    /* USER CODE BEGIN TIM2_IRQn 1 */

    /* USER CODE END TIM2_IRQn 1 */
}

/**
    * @brief This function handles TIM4 global interrupt.
    */
void TIM4_IRQHandler(void)
{
    /* USER CODE BEGIN TIM4_IRQn 0 */

    /* USER CODE END TIM4_IRQn 0 */
    HAL_TIM_IRQHandler(&htim4);
    /* USER CODE BEGIN TIM4_IRQn 1 */

    /* USER CODE END TIM4_IRQn 1 */
}

/**
    * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
    */
void TIM6_DAC_IRQHandler(void)
{
    /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

    /* USER CODE END TIM6_DAC_IRQn 0 */
    HAL_TIM_IRQHandler(&htim6);
    /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

    /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
    * @brief This function handles TIM7 global interrupt.
    */
void TIM7_IRQHandler(void)
{
    /* USER CODE BEGIN TIM7_IRQn 0 */
    /* USER CODE END TIM7_IRQn 0 */
    HAL_TIM_IRQHandler(&htim7);
    /* USER CODE BEGIN TIM7_IRQn 1 */
    /* USER CODE END TIM7_IRQn 1 */
}

/**
    * @brief This function handles UART5 global interrupt.
    * @note  指纹模块接收中断。旧工程没有实现这个函数，
    *        因此一旦启用 HAL_UART_Receive_IT 就会进默认死循环。
    */
void UART5_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart5);
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

