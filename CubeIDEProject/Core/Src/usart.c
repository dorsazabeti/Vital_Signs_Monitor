/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <string.h>

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
static uint8_t uart_rx_byte;
static char uart_rx_build_line[UART_RX_BUFFER_SIZE];
static char uart_rx_queue[UART_RX_QUEUE_DEPTH][UART_RX_BUFFER_SIZE];
static volatile uint16_t uart_rx_index = 0U;
static volatile uint8_t uart_rx_overflow = 0U;
static volatile uint8_t uart_rx_queue_head = 0U;
static volatile uint8_t uart_rx_queue_tail = 0U;
static volatile uint8_t uart_rx_queue_count = 0U;
static volatile uint8_t uart_receive_armed = 0U;
static volatile UART_RxStats uart_rx_stats;
UART_HandleTypeDef huart6;

/* USART1 init function */

void MX_USART1_UART_Init(void)
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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
  {
    uart_rx_stats.rearm_failures++;
    uart_receive_armed = 0U;
  }
  else
  {
    uart_receive_armed = 1U;
  }

  /* USER CODE END USART1_Init 2 */

}
/* USART6 init function */

void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = VCP_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(VCP_RX_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = VCP_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(VCP_TX_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspInit 0 */

  /* USER CODE END USART6_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART6;
    PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART6 clock enable */
    __HAL_RCC_USART6_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART6 GPIO Configuration
    PC7     ------> USART6_RX
    PC6     ------> USART6_TX
    */
    GPIO_InitStruct.Pin = ARDUINO_RX_D0_Pin|ARDUINO_TX_D1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN USART6_MspInit 1 */

  /* USER CODE END USART6_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    HAL_GPIO_DeInit(VCP_RX_GPIO_Port, VCP_RX_Pin);

    HAL_GPIO_DeInit(VCP_TX_GPIO_Port, VCP_TX_Pin);

  /* USER CODE BEGIN USART1_MspDeInit 1 */
    HAL_NVIC_DisableIRQ(USART1_IRQn);

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspDeInit 0 */

  /* USER CODE END USART6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART6_CLK_DISABLE();

    /**USART6 GPIO Configuration
    PC7     ------> USART6_RX
    PC6     ------> USART6_TX
    */
    HAL_GPIO_DeInit(GPIOC, ARDUINO_RX_D0_Pin|ARDUINO_TX_D1_Pin);

  /* USER CODE BEGIN USART6_MspDeInit 1 */

  /* USER CODE END USART6_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

uint8_t UART_ReadLine(char *destination, uint16_t destination_size)
{
  uint8_t line_available = 0U;
  uint32_t interrupt_state;

  if ((destination == NULL) || (destination_size == 0U))
  {
    return 0U;
  }

  interrupt_state = __get_PRIMASK();
  __disable_irq();

  if (uart_rx_queue_count != 0U)
  {
    const char *queued_line = uart_rx_queue[uart_rx_queue_tail];
    size_t length = strlen(queued_line);

    if (length >= destination_size)
    {
      length = destination_size - 1U;
    }

    memcpy(destination, queued_line, length);
    destination[length] = '\0';
    uart_rx_queue_tail = (uint8_t)((uart_rx_queue_tail + 1U) % UART_RX_QUEUE_DEPTH);
    uart_rx_queue_count--;
    line_available = 1U;
  }

  if (interrupt_state == 0U)
  {
    __enable_irq();
  }

  return line_available;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart_receive_armed = 0U;
    if (uart_rx_byte == '\n')
    {
      if ((uart_rx_overflow == 0U) && (uart_rx_index > 0U))
      {
        if (uart_rx_queue_count < UART_RX_QUEUE_DEPTH)
        {
          uart_rx_build_line[uart_rx_index] = '\0';
          memcpy(uart_rx_queue[uart_rx_queue_head], uart_rx_build_line,
                 uart_rx_index + 1U);
          uart_rx_queue_head = (uint8_t)((uart_rx_queue_head + 1U) %
                                         UART_RX_QUEUE_DEPTH);
          uart_rx_queue_count++;
          uart_rx_stats.received_lines++;
        }
        else
        {
          uart_rx_stats.dropped_lines++;
        }
      }
      else if (uart_rx_overflow != 0U)
      {
        uart_rx_stats.overflow_lines++;
      }

      uart_rx_index = 0U;
      uart_rx_overflow = 0U;
    }
    else if (uart_rx_byte != '\r')
    {
      if (uart_rx_overflow == 0U)
      {
        if (uart_rx_index < (UART_RX_BUFFER_SIZE - 1U))
        {
          uart_rx_build_line[uart_rx_index] = (char)uart_rx_byte;
          uart_rx_index++;
        }
        else
        {
          uart_rx_overflow = 1U;
        }
      }
    }

    if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) == HAL_OK)
    {
      uart_receive_armed = 1U;
    }
    else
    {
      uart_rx_stats.rearm_failures++;
    }
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart_receive_armed = 0U;
    uart_rx_index = 0U;
    uart_rx_overflow = 0U;
    uart_rx_stats.hardware_errors++;
  }
}

void UART_Service(void)
{
  if ((uart_receive_armed == 0U) &&
      (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) == HAL_OK))
  {
    uart_receive_armed = 1U;
  }
}

UART_RxStats UART_GetRxStats(void)
{
  UART_RxStats snapshot;
  uint32_t interrupt_state = __get_PRIMASK();

  __disable_irq();
  snapshot.received_lines = uart_rx_stats.received_lines;
  snapshot.dropped_lines = uart_rx_stats.dropped_lines;
  snapshot.overflow_lines = uart_rx_stats.overflow_lines;
  snapshot.hardware_errors = uart_rx_stats.hardware_errors;
  snapshot.rearm_failures = uart_rx_stats.rearm_failures;
  if (interrupt_state == 0U)
  {
    __enable_irq();
  }
  return snapshot;
}

/* USER CODE END 1 */
