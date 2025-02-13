/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

enum state{
  POR, // power on reset 
  SEND_BL_SIGNAL, 
  WAIT_SIGNAL_RESPONSE, 
  SEND_SIGNAL_ACK,
  //transfer app 
  WAIT_FW_PKG,
  SEND_FW_PCK_ACK,
  GOTO_APP, 
};

enum command{
  RESET_CMD,
  NEW_FW,
  TRANSFER_FW, 
}; 

enum fw_update_error{
   LENGTH_INVALID = 0x01,
   CRC_NOT_MATH,
   WRONG_FORMAT, 
   CMD_INVALID, 

};

enum fw_transfer_state{
  START_BYTE,
  LENGTH_BYTE,
  CMD_BYTE,
  DATA_BYTE,
  CRC1_BYTE,
  CRC2_BYTE,
};

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
