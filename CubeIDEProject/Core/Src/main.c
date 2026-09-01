/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "crc.h"
#include "dcmi.h"
#include "dma2d.h"
#include "eth.h"
#include "fatfs.h"
#include "i2c.h"
#include "ltdc.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spdifrx.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"
#include "fmc.h"
#include "images.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lvgl.h"
#include "lv_port_disp.h"

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

/* USER CODE BEGIN PV */
static lv_obj_t *heart_img;
static lv_obj_t *heart_rate_label;
static uint16_t heart_rate_value = 0U;
static lv_timer_t *heart_timer;
static lv_obj_t *spo2_gauge;
static lv_obj_t *spo2_arrow;
static lv_obj_t *spo2_label;
static lv_obj_t *bubble_img
static lv_obj_t *temperature_label;
static lv_obj_t *scenario_label;
static lv_obj_t *ecg_chart;
static lv_chart_series_t *ecg_series;
static lv_obj_t *error_label;
static uint8_t heart_frame = 0;
static const lv_image_dsc_t *heart_frames[] = {&Heart1, &Heart2, &Heart3, &Heart4, &Heart5, &Heart6, &Heart7, &Heart8};
static volatile uint8_t lvgl_initialized = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);

/* USER CODE BEGIN PFP */
static void Heart_Timer_Callback(lv_timer_t *timer); 
static lv_obj_t *Vitals_CreateCard(lv_obj_t *parent, int32_t x);
static lv_obj_t *Vitals_CreateLabel(lv_obj_t *parent, const char *text, lv_color_t color);
static void Vitals_CreateDashboard(lv_obj_t *screen);
void ECG_Update(int16_t ecg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define LCD_FRAME_BUFFER_ADDRESS  0xC0000000U
#define FMC_REGISTER_ADDRESS      0xA0000000U
#define LCD_WIDTH                 480U
#define LCD_HEIGHT                272U

static void SDRAM_MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = LCD_FRAME_BUFFER_ADDRESS;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Allow access to the FMC control registers hidden by MPU region 0. */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = FMC_REGISTER_ADDRESS;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static uint8_t SDRAM_Test(void)
{
  volatile uint32_t *sdram = (volatile uint32_t *)LCD_FRAME_BUFFER_ADDRESS;
  static const uint32_t offsets[] = {0U, 1U, 0x100U, 0x1000U, 0x10000U, 0x100000U, 0x1FFFFFU};
  static const uint32_t patterns[] = {0x00000000U, 0xFFFFFFFFU, 0xAAAAAAAAU, 0x55555555U, 0x12345678U, 0x89ABCDEFU, 0x0F0FF0F0U};

  for (uint32_t index = 0U; index < (sizeof(offsets) / sizeof(offsets[0])); index++)
  {
    sdram[offsets[index]] = patterns[index];
  }

  __DSB();

  for (uint32_t index = 0U; index < (sizeof(offsets) / sizeof(offsets[0])); index++)
  {
    if (sdram[offsets[index]] != patterns[index])
    {
      return 0U;
    }
  }

  return 1U;
}

static void LCD_ShowSDRAMError(void)
{
  __HAL_LTDC_LAYER_DISABLE(&hltdc, 0U);
  WRITE_REG(hltdc.Instance->BCCR, 0x00FF00FFU);
  __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  while (1)
  {
  }
}

static void LCD_ConfigureSDRAMLayer(void)
{
  LTDC_LayerCfgTypeDef layer = {0};

  layer.WindowX0 = 0U;
  layer.WindowX1 = LCD_WIDTH;
  layer.WindowY0 = 0U;
  layer.WindowY1 = LCD_HEIGHT;
  layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  layer.Alpha = 255U;
  layer.Alpha0 = 0U;
  layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  layer.FBStartAdress = LCD_FRAME_BUFFER_ADDRESS;
  layer.ImageWidth = LCD_WIDTH;
  layer.ImageHeight = LCD_HEIGHT;

  if (HAL_LTDC_ConfigLayer(&hltdc, &layer, 0U) != HAL_OK)
  {
    Error_Handler();
  }
}

void ECG_Update(int16_t ecg)
{
    if(ecg_series == NULL)
    {
        return;
    }

    lv_chart_set_next_value(
        ecg_chart,
        ecg_series,
        ecg
    );
}

static void Heart_Timer_Callback(lv_timer_t *timer) {
    lv_image_set_src(heart_img, heart_frames[heart_frame]);

    uint16_t hr = heart_rate_value;
    if (hr == 0U) {
      hr = 60U;
    }

    uint32_t frame_period = 60000U / (9U * (uint32_t)hr);
    if (heart_frame == 7U) {
      heart_frame = 0U;
      lv_timer_set_period(timer, frame_period * 2U);
    }
    else {
      heart_frame++;
      lv_timer_set_period(timer, frame_period);
    }
}

static lv_obj_t *Vitals_CreateCard(lv_obj_t *parent, int32_t x)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, 148, 196);
  lv_obj_set_pos(card, x, 38);
  lv_obj_set_scrollable(card, false);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x173D5BU), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x2A6687U), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  return card;
}

static lv_obj_t *Vitals_CreateLabel(lv_obj_t *parent, const char *text, lv_color_t color)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, color, 0);
  return label;
}

static int32_t Gauge_Angle(uint8_t spo2) {
  if (spo2 >= 100U) {
    return 1200;
  }

  if (spo2 >= 95U) {
    return 400 + ((int32_t)(spo2 - 95U) * 800) / 5;
  }

  if (spo2 >= 90U) {
    return -400 + ((int32_t)(spo2 - 90U) * 800) / 5;
  }

  if (spo2 >= 80U) {
    return -1200 + ((int32_t)(spo2 - 80U) * 800) / 10;
  }

  return -1200;
}

void Vitals_UpdateUI(uint16_t heart_rate, uint8_t spo2, int16_t temperature_tenths, const char *scenario)
{
  if ((heart_rate_label == NULL) || (spo2_label == NULL) ||
      (temperature_label == NULL) || (scenario_label == NULL))
  {
    return;
  }

  heart_rate_value = heart_rate;

  lv_label_set_text_fmt(heart_rate_label, "%u BPM", (unsigned int)heart_rate);
  lv_label_set_text_fmt(spo2_label, "%u %%", (unsigned int)spo2);
  lv_label_set_text_fmt(temperature_label, "%d.%d C",
                        (int)(temperature_tenths / 10),
                        (int)(temperature_tenths % 10));
  lv_label_set_text_fmt(scenario_label, "%s | LIVE UART", scenario);
  
  if ((heart_timer == NULL) || (heart_rate == 0U)) {
    return;
  }
  lv_timer_set_period(heart_timer, 60000U / ((uint32_t)heart_rate * 8U));

  if (spo2_arrow != NULL) {
    int32_t angle = Gauge_Angle(spo2);
    lv_image_set_rotation(spo2_arrow, angle);
  }

  if (bubble_img != NULL) {
    int32_t y;

    if (temperature_tenths <= 350) {
      y = 62;
    }
    else if (temperature_tenths >= 400) {
      y = 52;
    }
    else {
      y = 62 - ((temperature_tenths - 350) * 10) / 50;
    }

    lv_obj_set_y(bubble_img, y);
  }

  if (error_label != NULL) {
    bool abnormal = false;

    if (heart_rate < 60U) {
      lv_label_set_text(error_label, "WARNING: LOW HEART RATE");
      abnormal = true;
    }
    
    if (heart_rate > 100U) {
      lv_label_set_text(error_label, "WARNING: HIGH HEART RATE");
      abnormal = true;
    }

    if (spo2 < 95U) {
      lv_label_set_text(error_label, "WARNING: LOW BLOOD OXYGEN");
      abnormal = true;
    }

    if (temperature_tenths < 360){
      lv_label_set_text(error_label, "WARNING: LOW TEMPERATURE");
      abnormal = true;
    }
    
    if (temperature_tenths < 375){
      lv_label_set_text(error_label, "WARNING: HIGH TEMPERATURE");
      abnormal = true;
    }
  }

  if (abnormal) {
    // TODO: send notification
  }
}

static void Vitals_CreateDashboard(lv_obj_t *screen)
{
  const lv_color_t caption_color = lv_color_hex(0xB8D5E5U);
  lv_obj_t *heart_card = Vitals_CreateCard(screen, 8);
  lv_obj_t *temperature_card = Vitals_CreateCard(screen, 166);
  lv_obj_t *spo2_card = Vitals_CreateCard(screen, 324);
  lv_obj_t *title;
  lv_obj_t *caption;
  lv_obj_t *image;

  title = Vitals_CreateLabel(screen, "VITAL SIGNS MONITOR", lv_color_white());
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  caption = Vitals_CreateLabel(heart_card, "HEART RATE", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 8);

  heart_img = lv_image_create(heart_card);
  lv_image_set_src(heart_img, heart_frames[0]);
  lv_obj_set_pos(heart_img, 13, 32);

  heart_rate_label = Vitals_CreateLabel(heart_card, "-- BPM", lv_color_white());
  lv_obj_align(heart_rate_label, LV_ALIGN_BOTTOM_MID, 0, -12);

  caption = Vitals_CreateLabel(temperature_card, "TEMPERATURE", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 8);

  image = lv_image_create(temperature_card);
  lv_image_set_src(image, &Thermometer);
  lv_obj_set_pos(image, 7, 42);

  bubble_img = lv_image_create(temperature_card);
  lv_image_set_src(image, &Bubble);
  lv_obj_set_pos(image, 39, 59);

  temperature_label = Vitals_CreateLabel(temperature_card, "--.- C", lv_color_hex(0x102A43U));
  lv_obj_set_width(temperature_label, 78);
  lv_obj_set_style_text_align(temperature_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(temperature_label, 57, 79);

  caption = Vitals_CreateLabel(spo2_card, "BLOOD OXYGEN", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 8);

  image = lv_image_create(spo2_card);

  lv_image_set_src(image, &Gauge);

  lv_obj_set_pos(image, 32, 42);

  spo2_arrow = lv_image_create(spo2_card);

  lv_image_set_src(spo2_arrow, &GaugeArrow);

  lv_obj_set_pos(spo2_arrow, 32, 42);

  lv_image_set_pivot(spo2_arrow, 42, 42);

  lv_image_set_rotation(spo2_arrow, 0);

  image = lv_image_create(spo2_card);
  lv_image_set_src(image, &Gauge);
  lv_obj_set_pos(image, 32, 42);

  spo2_arrow = lv_image_create(spo2_card);
  lv_image_set_src(spo2_arrow, &GaugeArrow);
  lv_obj_set_pos(spo2_arrow, 32, 42);
  lv_image_set_pivot(spo2_arrow, 42, 42);
  lv_image_set_rotation(spo2_arrow, 0);

  spo2_label = Vitals_CreateLabel(spo2_card, "-- %", lv_color_white());
  lv_obj_align(spo2_label, LV_ALIGN_BOTTOM_MID, 0, -12);

  scenario_label = Vitals_CreateLabel(screen, "STARTING SIMULATION", caption_color);
  lv_obj_align(scenario_label, LV_ALIGN_BOTTOM_MID, 0, -7);

  error_label = Vitals_CreateLabel(screen, "", lv_color_hex(0xFF0000U));
  lv_obj_align(error_label, LV_ALIGN_BOTTOM_LEFT, 8, -7);

  ecg_chart = lv_chart_create(screen);

  lv_obj_set_style_bg_color(
      ecg_chart,
      lv_color_hex(0x102A43),
      0
  );

  lv_obj_set_style_bg_opa(
      ecg_chart,
      LV_OPA_COVER,
      0
  );

  lv_obj_set_size(ecg_chart, 460,220);
  lv_obj_align(ecg_chart, LV_ALIGN_CENTER,0,0);

  lv_chart_set_type(ecg_chart, LV_CHART_TYPE_LINE);

  lv_chart_set_point_count(ecg_chart, 100);

  lv_chart_set_range(
      ecg_chart,
      LV_CHART_AXIS_PRIMARY_Y,
      -1000,
      1000
  );

  ecg_series = lv_chart_add_series(
      ecg_chart,
      lv_color_hex(0xFFFFFF),
      LV_CHART_AXIS_PRIMARY_Y
  );
  lv_obj_set_style_line_width(
      ecg_chart,
      1,
      LV_PART_ITEMS
  );

  Vitals_UpdateUI(76U, 98U, 370, "Normal");
  lv_label_set_text(scenario_label, "Normal | WAITING FOR UART");
  heart_timer = lv_timer_create(Heart_Timer_Callback, 90, NULL);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  SDRAM_MPU_Config();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LTDC_Init();
  MX_FMC_Init();
  MX_USART1_UART_Init();
#if !LCD_BRINGUP_MODE
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_ETH_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
  MX_SDMMC1_SD_Init();
  MX_SPDIFRX_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_USART6_UART_Init();
  MX_FATFS_Init();
#endif
  /* USER CODE BEGIN 2 */
  if (SDRAM_Test() == 0U)
  {
    LCD_ShowSDRAMError();
  }

  __DSB();
  LCD_ConfigureSDRAMLayer();
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
  HAL_Delay(2000U);

  lv_init();
  lvgl_initialized = 1U;

  lv_port_disp_init();

  lv_obj_t * screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x102A43U), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

  Vitals_CreateDashboard(screen);

  lv_obj_invalidate(screen);
  lv_refr_now(NULL);
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  lv_obj_invalidate(screen);
  lv_refr_now(NULL);
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if ((htim->Instance == TIM6) && (lvgl_initialized != 0U))
  {
    lv_tick_inc(1U);
  }

  /* USER CODE END Callback 1 */
}

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
