#include "ui_dashboard.h"

#include "images.h"
#include "vitals_thresholds.h"

#define UI_CARD_NORMAL_COLOR   0x2A6687U
#define UI_CARD_WARNING_COLOR  0xFFB020U
#define UI_CARD_CRITICAL_COLOR 0xFF3B4EU
#define UI_CARD_STALE_COLOR    0x667788U
#define UI_CAPTION_COLOR       0xB8D5E5U
#define UI_BACKGROUND_COLOR    0x102A43U

static lv_obj_t *heart_card;
static lv_obj_t *temperature_card;
static lv_obj_t *spo2_card;
static lv_obj_t *heart_img;
static lv_obj_t *heart_rate_label;
static lv_obj_t *spo2_arrow;
static lv_obj_t *spo2_label;
static lv_obj_t *temperature_bar;
static lv_obj_t *temperature_label;
static lv_obj_t *status_label;
static lv_obj_t *ecg_chart;
static lv_chart_series_t *ecg_series;
static lv_timer_t *heart_timer;
static uint16_t heart_rate_value;
static uint8_t heart_frame;
static uint8_t data_available;

static const lv_image_dsc_t *heart_frames[] = {
  &Heart1, &Heart2, &Heart3, &Heart4, &Heart5, &Heart6, &Heart7, &Heart8
};

typedef enum
{
  UI_CARD_NORMAL = 0,
  UI_CARD_WARNING,
  UI_CARD_CRITICAL
} UI_CardState;

static lv_obj_t *UI_CreateLabel(lv_obj_t *parent, const char *text,
                                lv_color_t color)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, color, 0);
  return label;
}

static lv_obj_t *UI_CreateCard(lv_obj_t *parent, int32_t x)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, 148, 148);
  lv_obj_set_pos(card, x, 30);
  lv_obj_set_scrollable(card, false);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x173D5BU), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(UI_CARD_NORMAL_COLOR), 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  return card;
}

static void UI_SetCardState(lv_obj_t *card, UI_CardState state)
{
  uint32_t color = UI_CARD_NORMAL_COLOR;

  if (card != NULL)
  {
    if (state == UI_CARD_CRITICAL)
    {
      color = UI_CARD_CRITICAL_COLOR;
    }
    else if (state == UI_CARD_WARNING)
    {
      color = UI_CARD_WARNING_COLOR;
    }
    lv_obj_set_style_border_color(card, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(card, (state == UI_CARD_NORMAL) ? 2 : 3, 0);
  }
}

static void UI_SetAllCardsColor(uint32_t color)
{
  lv_obj_set_style_border_color(heart_card, lv_color_hex(color), 0);
  lv_obj_set_style_border_color(temperature_card, lv_color_hex(color), 0);
  lv_obj_set_style_border_color(spo2_card, lv_color_hex(color), 0);
}

static int32_t UI_GaugeAngle(uint8_t spo2)
{
  if (spo2 >= 100U)
  {
    return 1200;
  }
  if (spo2 >= 95U)
  {
    return 400 + ((int32_t)(spo2 - 95U) * 800) / 5;
  }
  if (spo2 >= 90U)
  {
    return -400 + ((int32_t)(spo2 - 90U) * 800) / 5;
  }
  if (spo2 >= 80U)
  {
    return -1200 + ((int32_t)(spo2 - 80U) * 800) / 10;
  }
  return -1200;
}

static void UI_HeartTimerCallback(lv_timer_t *timer)
{
  uint16_t heart_rate = heart_rate_value;
  uint32_t frame_period;

  if ((heart_img == NULL) || (timer == NULL))
  {
    return;
  }

  lv_image_set_src(heart_img, heart_frames[heart_frame]);
  heart_frame = (uint8_t)((heart_frame + 1U) % 8U);

  if ((data_available == 0U) || (heart_rate == 0U))
  {
    lv_timer_set_period(timer, 200U);
    return;
  }

  frame_period = 60000U / (8U * (uint32_t)heart_rate);
  if (frame_period < 30U)
  {
    frame_period = 30U;
  }
  lv_timer_set_period(timer, frame_period);
}

void Vitals_UpdateUI(uint16_t heart_rate, uint8_t spo2,
                     int16_t temperature_tenths, const char *scenario)
{
  UI_CardState heart_state;
  UI_CardState spo2_state;
  UI_CardState temperature_state;
  const char *safe_scenario = (scenario != NULL) ? scenario : "Unknown";
  int temperature_absolute = temperature_tenths;
  const char *temperature_sign = "";

  if ((heart_rate_label == NULL) || (spo2_label == NULL) ||
      (temperature_label == NULL) || (status_label == NULL))
  {
    return;
  }

  data_available = 1U;
  heart_rate_value = heart_rate;
  heart_state = ((heart_rate < VITALS_HR_CRITICAL_LOW) ||
                 (heart_rate > VITALS_HR_CRITICAL_HIGH))
                    ? UI_CARD_CRITICAL
                    : (((heart_rate < VITALS_HR_LOW) ||
                        (heart_rate > VITALS_HR_HIGH))
                           ? UI_CARD_WARNING
                           : UI_CARD_NORMAL);
  spo2_state = (spo2 < VITALS_SPO2_CRITICAL_LOW)
                   ? UI_CARD_CRITICAL
                   : ((spo2 < VITALS_SPO2_LOW) ? UI_CARD_WARNING
                                                : UI_CARD_NORMAL);
  temperature_state = ((temperature_tenths < VITALS_TEMP_CRITICAL_LOW) ||
                       (temperature_tenths >= VITALS_TEMP_CRITICAL_HIGH))
                          ? UI_CARD_CRITICAL
                          : (((temperature_tenths < VITALS_TEMP_LOW_TENTHS) ||
                              (temperature_tenths >= VITALS_TEMP_HIGH_TENTHS))
                                 ? UI_CARD_WARNING
                                 : UI_CARD_NORMAL);

  if (temperature_absolute < 0)
  {
    temperature_sign = "-";
    temperature_absolute = -temperature_absolute;
  }

  lv_label_set_text_fmt(heart_rate_label, "%u BPM", (unsigned int)heart_rate);
  lv_label_set_text_fmt(spo2_label, "%u %%", (unsigned int)spo2);
  lv_label_set_text_fmt(temperature_label, "%s%d.%d C", temperature_sign,
                        temperature_absolute / 10, temperature_absolute % 10);

  if (spo2_arrow != NULL)
  {
    lv_image_set_rotation(spo2_arrow, UI_GaugeAngle(spo2));
  }
  if (temperature_bar != NULL)
  {
    int32_t display_temperature = temperature_tenths;
    if (display_temperature < 320)
    {
      display_temperature = 320;
    }
    else if (display_temperature > 430)
    {
      display_temperature = 430;
    }
    lv_bar_set_value(temperature_bar, display_temperature, LV_ANIM_ON);
  }

  UI_SetCardState(heart_card, heart_state);
  UI_SetCardState(spo2_card, spo2_state);
  UI_SetCardState(temperature_card, temperature_state);

  if (spo2_state == UI_CARD_CRITICAL)
  {
    lv_label_set_text_fmt(status_label, "%s | CRITICAL: LOW OXYGEN", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_CRITICAL_COLOR), 0);
  }
  else if (heart_state == UI_CARD_CRITICAL)
  {
    lv_label_set_text_fmt(status_label, "%s | CRITICAL: HEART RATE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_CRITICAL_COLOR), 0);
  }
  else if (temperature_state == UI_CARD_CRITICAL)
  {
    lv_label_set_text_fmt(status_label, "%s | CRITICAL: TEMPERATURE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_CRITICAL_COLOR), 0);
  }
  else if (spo2_state == UI_CARD_WARNING)
  {
    lv_label_set_text_fmt(status_label, "%s | ALERT: LOW OXYGEN", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_WARNING_COLOR), 0);
  }
  else if (heart_rate < VITALS_HR_LOW)
  {
    lv_label_set_text_fmt(status_label, "%s | ALERT: LOW HEART RATE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_WARNING_COLOR), 0);
  }
  else if (heart_rate > VITALS_HR_HIGH)
  {
    lv_label_set_text_fmt(status_label, "%s | ALERT: HIGH HEART RATE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_WARNING_COLOR), 0);
  }
  else if (temperature_tenths < VITALS_TEMP_LOW_TENTHS)
  {
    lv_label_set_text_fmt(status_label, "%s | ALERT: LOW TEMPERATURE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_WARNING_COLOR), 0);
  }
  else if (temperature_tenths >= VITALS_TEMP_HIGH_TENTHS)
  {
    lv_label_set_text_fmt(status_label, "%s | ALERT: HIGH TEMPERATURE", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_WARNING_COLOR), 0);
  }
  else
  {
    lv_label_set_text_fmt(status_label, "%s | LIVE UART", safe_scenario);
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CAPTION_COLOR), 0);
  }

  if ((heart_timer != NULL) && (heart_rate != 0U))
  {
    uint32_t frame_period = 60000U / (8U * (uint32_t)heart_rate);
    lv_timer_set_period(heart_timer, (frame_period < 30U) ? 30U : frame_period);
  }
}

void Vitals_SetDataAvailable(uint8_t available)
{
  data_available = (available != 0U) ? 1U : 0U;
  if ((data_available == 0U) && (status_label != NULL))
  {
    heart_rate_value = 0U;
    lv_label_set_text(status_label, "NO DATA | CHECK UART");
    lv_obj_set_style_text_color(status_label, lv_color_hex(UI_CARD_STALE_COLOR), 0);
    UI_SetAllCardsColor(UI_CARD_STALE_COLOR);
    if (heart_timer != NULL)
    {
      lv_timer_set_period(heart_timer, 200U);
    }
  }
}

void ECG_Update(int16_t ecg_millivolts)
{
  if ((ecg_chart != NULL) && (ecg_series != NULL))
  {
    lv_chart_set_next_value(ecg_chart, ecg_series, ecg_millivolts);
  }
}

void UI_Dashboard_Init(lv_obj_t *screen)
{
  const lv_color_t caption_color = lv_color_hex(UI_CAPTION_COLOR);
  lv_obj_t *title;
  lv_obj_t *caption;
  lv_obj_t *image;

  if (screen == NULL)
  {
    return;
  }

  heart_card = UI_CreateCard(screen, 8);
  temperature_card = UI_CreateCard(screen, 166);
  spo2_card = UI_CreateCard(screen, 324);

  title = UI_CreateLabel(screen, "VITAL SIGNS MONITOR", lv_color_white());
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

  caption = UI_CreateLabel(heart_card, "HEART RATE", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 5);
  heart_img = lv_image_create(heart_card);
  lv_image_set_src(heart_img, heart_frames[0]);
  lv_image_set_scale(heart_img, 210U);
  lv_obj_set_pos(heart_img, 25, 24);
  heart_rate_label = UI_CreateLabel(heart_card, "-- BPM", lv_color_white());
  lv_obj_align(heart_rate_label, LV_ALIGN_BOTTOM_MID, 0, -5);

  caption = UI_CreateLabel(temperature_card, "TEMPERATURE", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 5);
  image = lv_image_create(temperature_card);
  lv_image_set_src(image, &Thermometer);
  lv_obj_set_pos(image, 12, 27);
  temperature_bar = lv_bar_create(temperature_card);
  lv_obj_set_size(temperature_bar, 18, 78);
  lv_obj_set_pos(temperature_bar, 76, 34);
  lv_bar_set_orientation(temperature_bar, LV_BAR_ORIENTATION_VERTICAL);
  lv_bar_set_range(temperature_bar, 320, 430);
  lv_bar_set_value(temperature_bar, 370, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(temperature_bar, lv_color_hex(0x254E68U), LV_PART_MAIN);
  lv_obj_set_style_bg_color(temperature_bar, lv_color_hex(0xFF5A5FU), LV_PART_INDICATOR);
  temperature_label = UI_CreateLabel(temperature_card, "--.- C", lv_color_white());
  lv_obj_align(temperature_label, LV_ALIGN_BOTTOM_MID, 20, -5);

  caption = UI_CreateLabel(spo2_card, "BLOOD OXYGEN", caption_color);
  lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 5);
  image = lv_image_create(spo2_card);
  lv_image_set_src(image, &Gauge);
  lv_obj_set_pos(image, 32, 28);
  spo2_arrow = lv_image_create(spo2_card);
  lv_image_set_src(spo2_arrow, &GaugeArrow);
  lv_obj_set_pos(spo2_arrow, 32, 28);
  lv_image_set_pivot(spo2_arrow, 42, 42);
  lv_image_set_rotation(spo2_arrow, 0);
  spo2_label = UI_CreateLabel(spo2_card, "-- %", lv_color_white());
  lv_obj_align(spo2_label, LV_ALIGN_BOTTOM_MID, 0, -5);

  ecg_chart = lv_chart_create(screen);
  lv_obj_set_size(ecg_chart, 464, 60);
  lv_obj_set_pos(ecg_chart, 8, 183);
  lv_obj_set_scrollable(ecg_chart, false);
  lv_obj_set_style_bg_color(ecg_chart, lv_color_hex(0x0B2539U), 0);
  lv_obj_set_style_bg_opa(ecg_chart, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(ecg_chart, lv_color_hex(UI_CARD_NORMAL_COLOR), 0);
  lv_obj_set_style_border_width(ecg_chart, 1, 0);
  lv_obj_set_style_radius(ecg_chart, 8, 0);
  lv_obj_set_style_pad_all(ecg_chart, 3, 0);
  lv_obj_set_style_line_opa(ecg_chart, LV_OPA_20, LV_PART_MAIN);
  lv_chart_set_type(ecg_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(ecg_chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_point_count(ecg_chart, 100U);
  lv_chart_set_div_line_count(ecg_chart, 3U, 5U);
  lv_chart_set_range(ecg_chart, LV_CHART_AXIS_PRIMARY_Y, -400, 1300);
  ecg_series = lv_chart_add_series(ecg_chart, lv_color_hex(0x57E389U),
                                   LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_set_style_line_width(ecg_chart, 2, LV_PART_ITEMS);

  status_label = UI_CreateLabel(screen, "WAITING FOR UART", caption_color);
  lv_obj_set_width(status_label, 464);
  lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -3);

  heart_timer = lv_timer_create(UI_HeartTimerCallback, 100U, NULL);
  Vitals_SetDataAvailable(0U);
}
