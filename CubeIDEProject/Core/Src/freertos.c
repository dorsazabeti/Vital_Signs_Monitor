/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usart.h"
#include <stdlib.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lvgl.h"

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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static uint8_t Vitals_ParseJson(const char *json, uint16_t *heart_rate,
                                uint8_t *spo2, int16_t *temperature_tenths,
                                char *scenario, uint16_t scenario_size);

static uint8_t Json_ParseECG(const char *json, int16_t *ecg);

extern void ECG_Update(int16_t ecg);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_USB_HOST_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 2 */
__weak void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 4096);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  char uart_line[UART_RX_BUFFER_SIZE];

  /* init code for USB_HOST */
#if !LCD_BRINGUP_MODE
  MX_USB_HOST_Init();
#endif
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
      if (UART_ReadLine(uart_line, sizeof(uart_line)) != 0U)
      {
          uint16_t heart_rate;
          uint8_t spo2;
          int16_t temperature_tenths;
          int16_t ecg;
          char scenario[24];

          if (Vitals_ParseJson(uart_line,
                              &heart_rate,
                              &spo2,
                              &temperature_tenths,
                              scenario,
                              sizeof(scenario)) != 0U)
          {
              Vitals_UpdateUI(
                  heart_rate,
                  spo2,
                  temperature_tenths,
                  scenario
              );

              if(Json_ParseECG(uart_line, &ecg) != 0U)
              {
                  ECG_Update(ecg);
              }
          }
      }


      uint32_t delay_ms = lv_timer_handler();

      if(delay_ms < 1U)
      {
          delay_ms = 1U;
      }
      else if(delay_ms > 20U)
      {
          delay_ms = 20U;
      }

      osDelay(delay_ms);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static const char *Json_FindValue(const char *json, const char *key)
{
  const char *value = strstr(json, key);

  if (value == NULL)
  {
    return NULL;
  }

  return value + strlen(key);
}

static uint8_t Json_ParseInteger(const char *json, const char *key, int32_t *result)
{
  const char *value = Json_FindValue(json, key);
  char *end;
  long parsed;

  if (value == NULL)
  {
    return 0U;
  }

  parsed = strtol(value, &end, 10);
  if (end == value)
  {
    return 0U;
  }

  *result = (int32_t)parsed;
  return 1U;
}

static uint8_t Json_ParseTemperature(const char *json, int16_t *temperature_tenths)
{
  const char *value = Json_FindValue(json, "\"temp\":");
  char *end;
  long whole;
  int32_t tenths;

  if (value == NULL)
  {
    return 0U;
  }

  whole = strtol(value, &end, 10);
  if (end == value)
  {
    return 0U;
  }

  tenths = (int32_t)(whole * 10L);
  if ((end[0] == '.') && (end[1] >= '0') && (end[1] <= '9'))
  {
    if (value[0] == '-')
    {
      tenths -= (int32_t)(end[1] - '0');
    }
    else
    {
      tenths += (int32_t)(end[1] - '0');
    }
  }

  *temperature_tenths = (int16_t)tenths;
  return 1U;
}

static uint8_t Json_ParseScenario(const char *json, char *scenario,
                                  uint16_t scenario_size)
{
  const char *value = Json_FindValue(json, "\"scenario\":\"");
  const char *end;
  size_t length;

  if ((value == NULL) || (scenario_size == 0U))
  {
    return 0U;
  }

  end = strchr(value, '"');
  if (end == NULL)
  {
    return 0U;
  }

  length = (size_t)(end - value);
  if ((length == 0U) || (length >= scenario_size))
  {
    return 0U;
  }

  memcpy(scenario, value, length);
  scenario[length] = '\0';
  return 1U;
}

static uint8_t Json_ParseECG(const char *json, int16_t *ecg)
{
    const char *value = strstr(json, "\"ecg\":");

    if(value == NULL)
    {
        return 0U;
    }

    value += 6;

    int32_t sign = 1;
    int32_t result = 0;


    if(*value == '-')
    {
        sign = -1;
        value++;
    }


    while((*value >= '0') && (*value <= '9'))
    {
        result = (result * 10) + (*value - '0');
        value++;
    }


    if(*value == '.')
    {
        value++;

        if((*value >= '0') && (*value <= '9'))
        {
            result = (result * 10) + (*value - '0');
        }
    }


    *ecg = (int16_t)(result * sign * 1000);

    return 1U;
}

static uint8_t Json_ParseECG(const char *json, int16_t *ecg);
static uint8_t Vitals_ParseJson(const char *json, uint16_t *heart_rate,
                                uint8_t *spo2, int16_t *temperature_tenths,
                                char *scenario, uint16_t scenario_size)
{
  int32_t parsed_heart_rate;
  int32_t parsed_spo2;
  int16_t parsed_temperature;

  if ((Json_ParseInteger(json, "\"hr\":", &parsed_heart_rate) == 0U) ||
      (Json_ParseInteger(json, "\"spo2\":", &parsed_spo2) == 0U) ||
      (Json_ParseTemperature(json, &parsed_temperature) == 0U) ||
      (Json_ParseScenario(json, scenario, scenario_size) == 0U))
  {
    return 0U;
  }

  if ((parsed_heart_rate < 30) || (parsed_heart_rate > 220) ||
      (parsed_spo2 < 50) || (parsed_spo2 > 100) ||
      (parsed_temperature < 320) || (parsed_temperature > 430))
  {
    return 0U;
  }

  *heart_rate = (uint16_t)parsed_heart_rate;
  *spo2 = (uint8_t)parsed_spo2;
  *temperature_tenths = parsed_temperature;
  return 1U;
}

/* USER CODE END Application */

