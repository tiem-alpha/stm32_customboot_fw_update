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
#include "app_config.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "crc16.h"

#define MAX_UART1_TX_BUFF 10
#define MAX_UART1_RX_BUFF 256

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

static HAL_StatusTypeDef write_data_to_flash_app(uint8_t *data,
                                                 uint16_t data_len, bool is_first_block);
static void goto_application(void);

uint8_t BootLoaderVersion[3] = {MAJOR, MINOR, BUILD};
static uint8_t state;
static app_data appData;
static uint16_t application_write_idx;
static volatile uint8_t rxIdx = 0;
static volatile uint8_t rxGetIdx = 0;
static uint8_t fwTransferState = 0;
uint8_t tx[MAX_UART1_TX_BUFF];
uint8_t rx[MAX_UART1_RX_BUFF];
static volatile uint8_t rxByte;
#if (LOG_ENABLE)
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
set to 'Yes') calls __io_putchar() */
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

  return ch;
}
#endif

void Start_UART_IT()
{
  HAL_UART_Receive_IT(&huart1, (uint8_t *)&rxByte, 1); // Nhận từng byte một bằng ngắt
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1)
  {
    // printf("%.2x", rxByte);
    rx[rxIdx++] = rxByte;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rxByte, 1);
  }
}

bool CheckResponse(uint8_t *buff)
{
  // AC - LENGTH - NEW_FW -  MAJOR - MINOR - BUILD - APP LEN H - APP LEN L - CRC H - CRC L
  uint16_t crc = 0;
  if (buff[0] != 0xAC)
  {
    printf("error start byte %.2x\r\n", buff[0]);
    return false;
  }

  if (buff[1] != 6)
  {
    printf("error len byte %.2x\r\n", buff[1]);
    return false;
  }

  if (buff[2] != NEW_FW)
  {
    printf("error command id byte %.2x\r\n", buff[2]);
    return false;
  }

  crc = crc_16((const unsigned char *)&buff[1], 7);
  if (((crc & 0xFF) != buff[9]) || (((crc >> 8) & 0xFF) != buff[8]))
  {
    printf("error crc id byte cal crc = %.4x  recv crc = %.4x\r\n", crc, *(uint16_t *)&buff[8]);
    return false;
  }
  appData.major = buff[3];
  appData.minor = buff[4];
  appData.build = buff[5];
  appData.length = buff[6] << 8;
  appData.length += buff[7];
  printf("Application length %d\r\n",appData.length);
  // printf("Message new fw is valid\r\n");
  return true;
}

void BLStateMachine()
{
  uint8_t errCnt = 0;
  HAL_StatusTypeDef ex;
  uint16_t curentAppOffet = 0;
  uint8_t lengthPck = 0; // number byte of message
  uint16_t crc = 0;
  uint32_t previous_time = 0;
  uint8_t dataIdx = 0;
  uint8_t remainDataByte = 0;
  while (1)
  {
    switch (state)
    {
    case POR:
      // SystemClock_Config();
      // MX_GPIO_Init();
      // MX_USART1_UART_Init();
      printf("Start boot loader %d.%d.%d \r\n", BootLoaderVersion[0], BootLoaderVersion[1], BootLoaderVersion[2]);
      state = SEND_BL_SIGNAL;
      break;

    case SEND_BL_SIGNAL:
      tx[0] = 0xAC;
      // printf("SEND_BL_SIGNAL \r\n");
      rxIdx = 0;
      memset(rx, 0, sizeof(rx));
      Start_UART_IT();
      HAL_UART_Transmit(&huart1, (uint8_t *)tx, 1, HAL_MAX_DELAY);
      state = WAIT_SIGNAL_RESPONSE;
      previous_time = HAL_GetTick();
      /* code */
      break;

    case WAIT_SIGNAL_RESPONSE: /// AC - len - NEW_FW - major - minor - build - app len high - app len low - crc high - crc low
      // printf("WAIT_SIGNAL_RESPONSE \r\n");
      if ((rxIdx >= 10) && (rxIdx >= 10))
      {
        // printf("Received 10 bytes\r\n");
        // for (int i = 0; i < 10; i++)
        // {
        //   printf("0x%.2x ", rx[i]);
        // }
        // printf("\r\n");
        if (CheckResponse(rx))
        {
          state = SEND_SIGNAL_ACK;
        }
        else
        {
          state = GOTO_APP;
          errCnt = 0;
        }
      }
      else
      {
        if (HAL_GetTick() - previous_time >= 1000)
        {                                
          previous_time = HAL_GetTick(); 
          // printf("time out\n");
          errCnt++;
          if (errCnt < 5)
          {
            state = SEND_BL_SIGNAL;
          }
          else
          {
            state = GOTO_APP;
            errCnt = 0;
          }
        }
      }
      break;

    case SEND_SIGNAL_ACK:
      // printf("SEND_SIGNAL_ACK \r\n");
      tx[0] = 0x55;
      rxIdx = 0;
      memset(rx, 0, sizeof(rx));
      previous_time = HAL_GetTick();
      HAL_UART_Transmit(&huart1, (uint8_t *)tx, 1, HAL_MAX_DELAY);

      lengthPck = 0;
      fwTransferState = START_BYTE;
      curentAppOffet = 0;
      state = WAIT_FW_PKG;
      break;

    case WAIT_FW_PKG:
      /* AC- length- fw transfer - data[] - crc h - crc l*/
      // printf("data %d\r\n",rxIdx);
      if (rxIdx > 0 && rxGetIdx < rxIdx)
      {
        switch (fwTransferState)
        {
        case START_BYTE:
          // printf("START_BYTE \r\n");
          if (rx[rxGetIdx] == 0xAC)
          {
            fwTransferState = LENGTH_BYTE;
          }
          break;
        case LENGTH_BYTE:

          lengthPck = rx[rxGetIdx];
          fwTransferState = CMD_BYTE;
          remainDataByte = lengthPck;
          // printf("LENGTH_BYTE %d\r\n", lengthPck);
          break;

        case CMD_BYTE:
          if (rx[rxGetIdx] == TRANSFER_FW)
          {
            // printf("CMD_BYTE\r\n");
            fwTransferState = DATA_BYTE;
            remainDataByte--;
            dataIdx = rxGetIdx + 1;
          }
          break;

        case DATA_BYTE:
          remainDataByte--;
          // printf("DATA_BYTE %d\r\n", remainDataByte);
          if (remainDataByte <= 0)
          {
            fwTransferState = CRC1_BYTE;
          }
          break;

        case CRC1_BYTE:
          // printf("CRC1_BYTE\r\n");
          fwTransferState = CRC2_BYTE;
          break;

        case CRC2_BYTE:
          // printf("CRC2_BYTE\r\n");
          crc = crc_16((const unsigned char *)&rx[dataIdx - 2], lengthPck + 1);
          if ((crc & 0xff) == rx[dataIdx + lengthPck] && ((crc >> 8) & 0xff) == rx[dataIdx - 1 + lengthPck])
          {
            // printf("success crc\r\n");
            // printf("write_data_to_flash_app \r\n");
            // for(int i =0; i< lengthPck - 1;i++){
            //   printf("%.2x",rx[dataIdx+i]);
            // }
            // printf("\r\n");
            ex = write_data_to_flash_app(&rx[dataIdx], lengthPck - 1, (curentAppOffet < lengthPck-1));
            if (ex != HAL_OK)
            {
              printf("Write error \r\n");
              state = GOTO_APP;
              break;
            }
            else
            {
              curentAppOffet += (lengthPck - 1);
              printf("Writing %d/%d\r\n",curentAppOffet, appData.length);
              state = SEND_FW_PCK_ACK;
            }
          }
          else
          {
            printf("Error crc\r\n");
          }
          break;

        default:
          break;
        }
        rxGetIdx++;
        previous_time = HAL_GetTick();
      }
      else
      {
        if (HAL_GetTick() - previous_time >= 1000)
        {                                // Kiểm tra nếu đã trôi qua 1000ms
          previous_time = HAL_GetTick(); // Cập nhật thời điểm hiện tại
          printf("Time out\n");
          state = GOTO_APP;
        }
      }

      break;

    case SEND_FW_PCK_ACK:
      tx[0] = 0x55;
      // clear the memory
      lengthPck = 0;
      rxGetIdx = 0;
      fwTransferState = START_BYTE;
      memset(rx, 0, MAX_UART1_RX_BUFF);
      rxIdx = 0;

      if (curentAppOffet >= appData.length)
      {
        printf("Receive full firmware \r\n");
        HAL_UART_Transmit(&huart1, (uint8_t *)tx, 1, HAL_MAX_DELAY);
        state = GOTO_APP;
        break;
      }
      else
      {
        
        state = WAIT_FW_PKG;
        HAL_UART_Transmit(&huart1, (uint8_t *)tx, 1, HAL_MAX_DELAY);
        previous_time = HAL_GetTick();
      }
      break;

    case GOTO_APP:
      
      return; 
      break;

    default:
      state = POR;
      break;
    }
  }
}

static HAL_StatusTypeDef write_data_to_flash_app(uint8_t *data,
                                                 uint16_t data_len, bool is_first_block)
{
  printf("write %d, %d\r\n",data_len, is_first_block);
  HAL_StatusTypeDef ret;
  do
  {
    ret = HAL_FLASH_Unlock();
    if (ret != HAL_OK)
    {
      printf("error HAL_FLASH_Unlock\r\n");
      break;
    }
    // No need to erase every time. Erase only the first time.
    if (is_first_block)
    {
      printf("Erasing the Flash memory...\r\n");
      // Erase the Flash
      FLASH_EraseInitTypeDef EraseInitStruct;
      uint32_t SectorError;
      EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
      EraseInitStruct.PageAddress = APP_START_ADDRESS;
      EraseInitStruct.NbPages = 44; // 47 Pages
      ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
      if (ret != HAL_OK)
      {
        break;
      }
      application_write_idx = 0;
    }

    for (int i = 0; i < data_len / 2; i++)
    {
      uint16_t halfword_data = data[i * 2] | (data[i * 2 + 1] << 8);
      ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              (APP_START_ADDRESS + application_write_idx),
                              halfword_data);
      if (ret == HAL_OK)
      {
        // update the data count
        application_write_idx += 2;
      }
      else
      {
        printf("Flash Write Error...HALT!!!\r\n");
        break;
      }
    }
    if (ret != HAL_OK)
    {
      break;
    }
    ret = HAL_FLASH_Lock();
    if (ret != HAL_OK)
    {
      break;
    }
  } while (false);
  return ret;
}

static void goto_application(void)
{
  printf("Jump to Application...\r\n");
  void (*app_reset_handler)(void) = (void *)(*((volatile uint32_t *)(APP_START_ADDRESS + 4U)));
  if (app_reset_handler == (void *)0xFFFFFFFF)
  {
    printf("Invalid Application... HALT!!!\r\n");
    while (1)
      ;
  }
     // Tắt ngắt
     __disable_irq();
     SysTick->CTRL = 0; // Tắt SysTick
     SysTick->LOAD = 0;
     SysTick->VAL  = 0;
 
     // Cấu hình lại Vector Table
     SCB->VTOR = APP_START_ADDRESS; 
 
     // Đặt Stack Pointer mới
     __set_MSP(*(volatile uint32_t *)APP_START_ADDRESS);
 
     // Bật ngắt lại
     __enable_irq();
  //
  app_reset_handler(); // call the app reset handler
}

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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  BLStateMachine();
  goto_application();
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

#ifdef USE_FULL_ASSERT
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
