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
#include "fonts.h"
#include "SH1106.h"
#include "tm_stm32f4_mfrc522.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_LOGS 100
#define MAX_KEYS 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t uart_rx;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Hàm chuyển đổi binary sang BCD
uint8_t bin2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

// Hàm chuyển đổi BCD sang binary
uint8_t bcd2bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}
struct Time {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint8_t year;
};


// Hàm ghi thời gian vào DS1307
    void SetTime(struct Time *time) {
        struct Time bcdTime;

        bcdTime.sec = bin2bcd(time->sec) & 0x7F; // Clear CH bit (bit7)
        bcdTime.min = bin2bcd(time->min);
        bcdTime.hour = bin2bcd(time->hour) & 0x3F; // 24-hour mode (bit6=0)
        bcdTime.weekday = bin2bcd(time->weekday);
        bcdTime.day = bin2bcd(time->day);
        bcdTime.month = bin2bcd(time->month);
        bcdTime.year = bin2bcd(time->year);

        HAL_I2C_Mem_Write(&hi2c3, 0xD0, 0, 1, (uint8_t *)&bcdTime, 7, 1000);
    }

    // Hàm đọc thời gian từ DS1307 (đã cập nhật xử lý BCD)
    void GetTime(struct Time *time) {
        struct Time bcdTime;
        HAL_I2C_Mem_Read(&hi2c3, 0xD1, 0, 1, (uint8_t *)&bcdTime, 7, 1000);

        time->sec = bcd2bin(bcdTime.sec & 0x7F); // Mask out CH bit
        time->min = bcd2bin(bcdTime.min);
        time->hour = bcd2bin(bcdTime.hour & 0x3F); // Mask out 12/24 mode bit
        time->weekday = bcd2bin(bcdTime.weekday);
        time->day = bcd2bin(bcdTime.day);
        time->month = bcd2bin(bcdTime.month);
        time->year = bcd2bin(bcdTime.year);
    }


uint8_t authKeys[MAX_KEYS][5];
int authCount = 1;

uint8_t cardID[5];
char buf[100];

// Hàm kiểm tra mã thẻ hợp lệ
int isAuthorized(uint8_t *id) {
    for (int i = 0; i < authCount; i++) {
        if (memcmp(id, authKeys[i], 5) == 0)
            return 1;
    }
    return 0;
}
typedef struct {
    uint8_t time[3];      // [giây, phút, giờ]
    uint8_t cardID[5];    // mã thẻ RFID
} DoorLog;
DoorLog logs[MAX_LOGS];
uint8_t logCount = 0;
void save_log(uint8_t *cardID, uint8_t *time) {
    if (logCount < MAX_LOGS) {
        memcpy(logs[logCount].cardID, cardID, 5);
        memcpy(logs[logCount].time, time, 3); // time[0]=sec, time[1]=min, time[2]=hour
        logCount++;
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (uart_rx == 'L') {
            show_logs();  // Gửi toàn bộ log khi nhận ký tự 'L'
        }

        // Nhận lại tiếp ký tự tiếp theo
        HAL_UART_Receive_IT(&huart1, &uart_rx, 1);
    }
}

void show_logs() {
    char buffer[100];
    for (int i = 0; i < logCount; i++) {
        sprintf(buffer, "Log %02d: %02X%02X%02X%02X%02X at %02d:%02d:%02d\r\n", i + 1,
                logs[i].cardID[0], logs[i].cardID[1], logs[i].cardID[2],
                logs[i].cardID[3], logs[i].cardID[4],
                logs[i].time[2], logs[i].time[1], logs[i].time[0]);
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
    }
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C3_Init();
  MX_SPI4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  TM_MFRC522_Init();
//  char buf[100];
//  uint8_t X = 0, Y = 0;
//  SH1106_Init ();
//
        char buff[30];
//
      uint8_t defaultCard[5] = {0xA3, 0x1E, 0x31, 0xB9, 0x35};
      memcpy(authKeys[0], defaultCard, 5);
      authCount = 1;
      struct Time currentTime = {
           .sec = 50,
           .min = 59,
           .hour = 23,
           .weekday = 5,
           .day = 17,
           .month = 4,
           .year = 25
       };
       SetTime(&currentTime);  // Gọi một lần để set thời gian ban đầu



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_UART_Receive_IT(&huart1, &uart_rx, 1); // bắt đầu nhận UART
	  GetTime(&currentTime);
      SH1106_Fill(0);

	          if (TM_MFRC522_Check(cardID) == MI_OK) {
	              HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);

	              if (isAuthorized(cardID)) {
	                  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
	                  SH1106_GotoXY(10, 10);
	                  SH1106_Puts("Welcome", &Font_11x18, 1);
	                  SH1106_UpdateScreen();
	                  struct Time readTime;
	                  GetTime(&readTime);
	                  sprintf(buff, "%02d:%02d:%02d - %02d - %02d/%02d/%02d\r\n",
	                                  readTime.hour,
	                                  readTime.min,
	                                  readTime.sec,
	                                  readTime.weekday,
	                                  readTime.day,
	                                  readTime.month,
	                                  readTime.year);
	                          HAL_Delay(500);
	                                    HAL_UART_Transmit(&huart1, (uint8_t*)buff, strlen(buff), 1000);
	                                    HAL_Delay(500);
	                                    char time[3];
	              	                  sprintf(time, "%02d:%02d:%02d - %02d - %02d/%02d/%02d\r\n",
	              	                                  readTime.hour,
	              	                                  readTime.min,
	              	                                  readTime.sec,
	              	                                  readTime.weekday,
	              	                                  readTime.day,
	              	                                  readTime.month,
	              	                                  readTime.year);
	                                    save_log(cardID, time);

	              } else {
	                  SH1106_GotoXY(10, 10);
	                  SH1106_Puts("Rejected", &Font_11x18, 1);
	                  SH1106_UpdateScreen();
	              }
	              HAL_Delay(1000);
	          } else {
	              HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
	              HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
	          }

	          HAL_Delay(200);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 400000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

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
  huart1.Init.BaudRate = 115200;
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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PG13 PG14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
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
