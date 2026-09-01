#include "vitals_parser.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *VitalsParser_FindValue(const char *json, const char *key)
{
  const char *value;

  if ((json == NULL) || (key == NULL))
  {
    return NULL;
  }
  value = strstr(json, key);
  return (value != NULL) ? value + strlen(key) : NULL;
}

static uint8_t VitalsParser_IsNumberEnd(char value)
{
  return ((value == ',') || (value == '}') || (value == '\0')) ? 1U : 0U;
}

static uint8_t VitalsParser_Integer(const char *json, const char *key,
                                    int32_t *result)
{
  const char *value = VitalsParser_FindValue(json, key);
  char *end;
  long parsed;

  if ((value == NULL) || (result == NULL))
  {
    return 0U;
  }
  parsed = strtol(value, &end, 10);
  if ((end == value) || (VitalsParser_IsNumberEnd(*end) == 0U) ||
      (parsed < INT32_MIN) || (parsed > INT32_MAX))
  {
    return 0U;
  }
  *result = (int32_t)parsed;
  return 1U;
}

static uint8_t VitalsParser_FixedPoint(const char *json, const char *key,
                                       uint16_t scale, int32_t *result)
{
  const char *value = VitalsParser_FindValue(json, key);
  uint8_t negative = 0U;
  uint8_t digits = 0U;
  uint32_t whole = 0U;
  uint32_t fraction = 0U;
  uint32_t fraction_scale = scale;
  uint32_t scaled;

  if ((value == NULL) || (result == NULL) || (scale == 0U))
  {
    return 0U;
  }
  if (*value == '-')
  {
    negative = 1U;
    value++;
  }
  else if (*value == '+')
  {
    value++;
  }

  while ((*value >= '0') && (*value <= '9'))
  {
    if (whole > 100000U)
    {
      return 0U;
    }
    whole = (whole * 10U) + (uint32_t)(*value - '0');
    value++;
    digits++;
  }
  if (digits == 0U)
  {
    return 0U;
  }

  if (*value == '.')
  {
    value++;
    while ((*value >= '0') && (*value <= '9'))
    {
      if (fraction_scale > 1U)
      {
        fraction_scale /= 10U;
        fraction += (uint32_t)(*value - '0') * fraction_scale;
      }
      value++;
    }
  }

  if (VitalsParser_IsNumberEnd(*value) == 0U)
  {
    return 0U;
  }

  scaled = whole * scale + fraction;
  if (scaled > 2147483647UL)
  {
    return 0U;
  }
  *result = negative ? -(int32_t)scaled : (int32_t)scaled;
  return 1U;
}

static uint8_t VitalsParser_Scenario(const char *json, char *scenario,
                                     uint16_t scenario_size)
{
  const char *value = VitalsParser_FindValue(json, "\"scenario\":\"");
  const char *end;
  size_t length;

  if ((value == NULL) || (scenario == NULL) || (scenario_size == 0U))
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

uint8_t VitalsParser_Parse(const char *json, VitalsFrame *frame)
{
  int32_t heart_rate;
  int32_t spo2;
  int32_t temperature_tenths;
  int32_t ecg_millivolts;
  VitalsFrame parsed = {0};

  if ((json == NULL) || (frame == NULL) || (json[0] != '{'))
  {
    return 0U;
  }

  if ((VitalsParser_Integer(json, "\"hr\":", &heart_rate) == 0U) ||
      (VitalsParser_Integer(json, "\"spo2\":", &spo2) == 0U) ||
      (VitalsParser_FixedPoint(json, "\"temp\":", 10U,
                               &temperature_tenths) == 0U) ||
      (VitalsParser_FixedPoint(json, "\"ecg\":", 1000U,
                               &ecg_millivolts) == 0U) ||
      (VitalsParser_Scenario(json, parsed.scenario,
                             sizeof(parsed.scenario)) == 0U))
  {
    return 0U;
  }

  if ((heart_rate < 30) || (heart_rate > 220) ||
      (spo2 < 50) || (spo2 > 100) ||
      (temperature_tenths < 320) || (temperature_tenths > 430) ||
      (ecg_millivolts < -2000) || (ecg_millivolts > 3000))
  {
    return 0U;
  }

  parsed.heart_rate = (uint16_t)heart_rate;
  parsed.spo2 = (uint8_t)spo2;
  parsed.temperature_tenths = (int16_t)temperature_tenths;
  parsed.ecg_millivolts = (int16_t)ecg_millivolts;
  *frame = parsed;
  return 1U;
}
