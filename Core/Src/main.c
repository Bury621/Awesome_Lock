/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "App.h"
#include "Debug.h"
#include "app_util.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_UART5_Init(void);
static void MX_TIM2_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*
 * 寮€鏈烘墦鍗板浣嶅師鍥狅紝渚夸簬瀹氫綅鈥滆嚜鍔ㄩ噸鍚€濓細
 *   IWDG  -> 涓诲惊鐜秴杩囩害 10s 娌″杺鐙楋紙鍗℃/琚腑鏂タ姝伙級
 *   POR   -> 姝ｅ父涓婄數
 *   PIN   -> 澶嶄綅鑴?涓嬭浇鍣ㄥ浣?
 *   BOR   -> 渚涚數璺岃惤
 *   SFTRST-> 杞欢澶嶄綅
 */
static void Report_Reset_Cause(void)
{
    volatile AppFaultRecord *record = App_GetFaultRecord();

    uart_printf("\r\n=== BOOT ===\r\n");

    if ((record->magic == APP_FAULT_RECORD_MAGIC) &&
        (record->code_inv == ~record->code))
    {
        uart_printf("PREV FAULT code=%lu (1=Hard 2=Mem 3=Bus 4=Usage 5=NMI 6=ErrorHandler)\r\n",
                    (unsigned long)record->code);
    }
    record->magic = 0U;
    record->code = 0U;
    record->code_inv = 0U;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        uart_printf("RESET: IWDG (main loop stuck / watchdog)\r\n");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
    {
        uart_printf("RESET: WWDG\r\n");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET)
    {
        uart_printf("RESET: BOR (power dip)\r\n");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
    {
        uart_printf("RESET: Software\r\n");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
    {
        uart_printf("RESET: Pin / debugger\r\n");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
    {
        uart_printf("RESET: Power-on\r\n");
    }
    else
    {
        uart_printf("RESET: unknown\r\n");
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
	/*
	 * 璇存槑锛?
	 * 鏃х増涓轰簡缁曞紑鈥滀笂鐢典腑鏂棭浜庡璁惧垵濮嬪寲鈥濈殑闂锛岄噰鐢ㄤ簡
	 * 鈥滃厛鍒濆鍖?GPIO 鈫?鍏抽棴 EXTI 鈫?鍒濆鍖栧璁?鈫?鍐嶆墦寮€ EXTI鈥濈殑 hack銆?
	 * 鏂扮増涓柇閲屽彧鎶曢€掍簨浠讹紝涓嶅啀璁块棶浠讳綍澶栬锛屽洜姝や繚鎸?CubeMX 榛樿椤哄簭鍗冲彲銆?
	 */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_SPI2_Init();
    MX_TIM6_Init();
    MX_TIM7_Init();
    MX_USART1_UART_Init();
    MX_UART5_Init();
    MX_TIM2_Init();
    MX_IWDG_Init();
    MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	/* 缁欐寚绾规ā鍧椼€侀敭鐩樼瓑澶栭儴鍣ㄤ欢鐣欏嚭涓婄數绋冲畾鏃堕棿 */
	HAL_Delay(100);
	Report_Reset_Cause();
	HAL_Delay(200);
	App_Init();
  /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
		App_Running();
    }
    /* USER CODE END 3 */
}

/**
    * @brief System Clock Configuration
    * @retval None
    */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
    */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
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
}

/**
    * @brief I2C1 Initialization Function
    * @param None
    * @retval None
    */
static void MX_I2C1_Init(void)
{

    /* USER CODE BEGIN I2C1_Init 0 */
    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */

}

/**
    * @brief IWDG Initialization Function
    * @param None
    * @retval None
    */
static void MX_IWDG_Init(void)
{

    /* USER CODE BEGIN IWDG_Init 0 */

    /* USER CODE END IWDG_Init 0 */

    /* USER CODE BEGIN IWDG_Init 1 */

    /* USER CODE END IWDG_Init 1 */
    /*
     * IWDG 绾?10s 瓒呮椂锛圠SI 32kHz / 256 / 1250 鈮?10s锛夈€?
     * 蹇呴』澶т簬鏈€闀跨殑鍚堟硶闃诲鎿嶄綔锛歋ector5锛?28KB锛塅lash 鎿﹂櫎鏈€鍧忕害 4s銆?
     * 鐪嬮棬鐙楀彧鍦ㄤ富寰幆鍠傦紝浠讳綍浠诲姟鍗℃閮戒細鑷姩澶嶄綅锛岃€屼笉鏄儚鏃х増閭ｆ牱
     * 鍦?TIM4 涓柇閲屽杺鐙椼€佷富寰幆姝讳簡杩樿兘鈥滃亣娲烩€濄€?
     */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload = 1249;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN IWDG_Init 2 */

    /* USER CODE END IWDG_Init 2 */

}

/**
    * @brief SPI2 Initialization Function
    * @param None
    * @retval None
    */
static void MX_SPI2_Init(void)
{

    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */

    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI2_Init 2 */

    /* USER CODE END SPI2_Init 2 */

}

/**
    * @brief TIM2 Initialization Function
    * @param None
    * @retval None
    */
static void MX_TIM2_Init(void)
{

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 8399;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 9999;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */

}

/**
    * @brief TIM4 Initialization Function
    * @param None
    * @retval None
    */
static void MX_TIM4_Init(void)
{

    /* USER CODE BEGIN TIM4_Init 0 */

    /* USER CODE END TIM4_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM4_Init 1 */

    /* USER CODE END TIM4_Init 1 */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 41999;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 3999;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM4_Init 2 */

    /* USER CODE END TIM4_Init 2 */

}

/**
    * @brief TIM6 Initialization Function
    * @param None
    * @retval None
    */
static void MX_TIM6_Init(void)
{

    /* USER CODE BEGIN TIM6_Init 0 */

    /* USER CODE END TIM6_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM6_Init 1 */

    /* USER CODE END TIM6_Init 1 */
    /*
     * TIM6 浣滀负姝ヨ繘鐢垫満 1us 鏃跺熀锛?
     * APB1 瀹氭椂鍣ㄦ椂閽?84MHz锛?4MHz / (83 + 1) = 1MHz銆?
     * 杩欐牱 stepper_motor 閲岀殑 step_us 鍙傛暟灏辨槸鐪熷疄鐨勫井绉掓暟锛?
     * 涓嶅啀鍍忔棫鐗堥偅鏍封€? 涓鏁?= 2us鈥濓紝瀵艰嚧閰嶇疆鍊间笌瀹為檯閫熷害宸竴鍊嶃€?
     */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 167;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 199;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM6_Init 2 */

    /* USER CODE END TIM6_Init 2 */

}

/**
    * @brief TIM7 Initialization Function
    * @param None
    * @retval None
    */
static void MX_TIM7_Init(void)
{

    /* USER CODE BEGIN TIM7_Init 0 */

    /* USER CODE END TIM7_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM7_Init 1 */

    /* USER CODE END TIM7_Init 1 */
    htim7.Instance = TIM7;
    htim7.Init.Prescaler = 16799;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.Period = 999;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM7_Init 2 */

    /* USER CODE END TIM7_Init 2 */

}

/**
    * @brief UART5 Initialization Function
    * @param None
    * @retval None
    */
static void MX_UART5_Init(void)
{

    /* USER CODE BEGIN UART5_Init 0 */

    /* USER CODE END UART5_Init 0 */

    /* USER CODE BEGIN UART5_Init 1 */

    /* USER CODE END UART5_Init 1 */
    huart5.Instance = UART5;
    huart5.Init.BaudRate = 57600;
    huart5.Init.WordLength = UART_WORDLENGTH_8B;
    huart5.Init.StopBits = UART_STOPBITS_1;
    huart5.Init.Parity = UART_PARITY_NONE;
    huart5.Init.Mode = UART_MODE_TX_RX;
    huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart5.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart5) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN UART5_Init 2 */

    /* USER CODE END UART5_Init 2 */

}

/**
    * @brief USART1 Initialization Function
    * @param None
    * @retval None
    */
static void MX_USART1_UART_Init(void)
{

    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */

}

/**
    * @brief GPIO Initialization Function
    * @param None
    * @retval None
    */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, Led1_Pin|Led2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, MOTOR_IN1_Pin|MOTOR_IN2_Pin|MOTOR_IN3_Pin|MOTOR_IN4_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, LCD_RES_Pin|LCD_DC_Pin|LCD_CS_Pin|LCD_BL_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : PA0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    /*
     * 浜轰綋鎰熷簲妯″潡鏈帴鏃?PA0 浼氭偓绌猴紝鍙兘浜х敓澶ч噺杈规部涓柇銆?
     * 鍔犱笂涓嬫媺锛岀┖闂叉椂淇濇寔浣庣數骞筹紝閬垮厤 EXTI0 涓柇椋庢毚鎷栨涓诲惊鐜€?
     */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : Led1_Pin Led2_Pin */
    GPIO_InitStruct.Pin = Led1_Pin|Led2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : Hall_IRQ_Pin */
    /*
     * 闂ㄧ/闇嶅皵鍙仛鏅€氳緭鍏ワ紝涓嶅啀寮€ EXTI銆?
     * 瀹炴満鎶撳埌鐢垫満杞姩鏃?PE7 鐨?EXTI7 浼氭寔缁Е鍙戯紝EXTI9_5 鎶㈠崰 SysTick锛?
     * 涓诲惊鐜?鐪嬮棬鐙楄楗挎锛岃〃鐜颁负寮€闂?鍏抽棬鍒颁竴鍗婂崱姝诲苟閲嶅惎銆?
     * 鐩墠涓氬姟灞傚苟娌℃湁浣跨敤 APP_EVENT_DOOR_SENSE锛屾墍浠ョ洿鎺ュ叧鎺夎繖涓腑鏂簮銆?
     */
    GPIO_InitStruct.Pin = Hall_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(Hall_IRQ_GPIO_Port, &GPIO_InitStruct);

    /* 纭繚 EXTI line7 涓嶄細鍥犱负鍘嗗彶閰嶇疆/璋冭瘯鍣ㄧ姸鎬佺户缁Е鍙?*/
    EXTI->IMR &= ~(1UL << 7);
    EXTI->EMR &= ~(1UL << 7);
    EXTI->RTSR &= ~(1UL << 7);
    EXTI->FTSR &= ~(1UL << 7);
    EXTI->PR = (1UL << 7);

    /*Configure GPIO pin : FP_IRQ_Pin */
    GPIO_InitStruct.Pin = FP_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(FP_IRQ_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : MOTOR_IN1_Pin MOTOR_IN2_Pin MOTOR_IN3_Pin MOTOR_IN4_Pin */
    GPIO_InitStruct.Pin = MOTOR_IN1_Pin|MOTOR_IN2_Pin|MOTOR_IN3_Pin|MOTOR_IN4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : LCD_RES_Pin LCD_DC_Pin LCD_CS_Pin LCD_BL_Pin */
    GPIO_InitStruct.Pin = LCD_RES_Pin|LCD_DC_Pin|LCD_CS_Pin|LCD_BL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : KEYBOARD_IRQ_Pin */
    GPIO_InitStruct.Pin = KEYBOARD_IRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEYBOARD_IRQ_GPIO_Port, &GPIO_InitStruct);

    /*
     * 涓柇浼樺厛绾у垎缁勶細NVIC_PRIORITYGROUP_4锛堣 stm32f4xx_hal_msp.c锛夈€?
     *   TIM6    = 0锛堢數鏈鸿妭鎷嶆渶楂橈紝閬垮厤琚?1ms tick 鍙嶅鎵撴柇锛?
     *   SysTick = 1锛堥珮浜?UART5/EXTI锛屼繚璇?tick 鍜?HAL 瓒呮椂涓嶄細琚閮ㄤ腑鏂タ姝伙級
     *   UART5   = 2锛堟寚绾规帴鏀讹級
     *   EXTI    = 3锛堟寜閿?鎸囩汗瑙︽懜/浜轰綋鎰熷簲锛?
     * 鍓嶆彁锛氭墍鏈?ISR 閲岄兘涓嶈皟鐢ㄥ甫 tick 瓒呮椂鐨勯樆濉?HAL 鍑芥暟銆?
     */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 3U, 0U);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3U, 0U);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 涓嶈姘镐箙鍏充腑鏂寰幆锛氬浣嶈绯荤粺鎭㈠锛屼覆鍙ｇ暀涓嬫爣璁?*/
  volatile AppFaultRecord *record = App_GetFaultRecord();
  record->code = APP_FAULT_ERRORHANDLE;
  record->code_inv = ~APP_FAULT_ERRORHANDLE;
  record->magic = APP_FAULT_RECORD_MAGIC;
  __DSB();
  uart_printf("[ERROR_HANDLER]\r\n");
  __DSB();
  NVIC_SystemReset();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

