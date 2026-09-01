#include "network_alert_config.h"
#include "network_alert_logic.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static NetworkAlertVitals MakeVitals(uint16_t hr, uint8_t spo2, int16_t temp,
                                     const char *scenario)
{
  NetworkAlertVitals vitals = {0};
  vitals.heart_rate = hr;
  vitals.spo2 = spo2;
  vitals.temperature_tenths = temp;
  strncpy(vitals.scenario, scenario, sizeof(vitals.scenario) - 1U);
  return vitals;
}

static NetworkAlertEvent Trigger(NetworkAlertLogic *logic,
                                 const NetworkAlertVitals *vitals,
                                 uint32_t start_ms)
{
  NetworkAlertEvent event;
  assert(NetworkAlertLogic_Evaluate(logic, vitals, start_ms, &event) == 0U);
  assert(NetworkAlertLogic_Evaluate(logic, vitals, start_ms + 10U, &event) == 0U);
  assert(NetworkAlertLogic_Evaluate(logic, vitals, start_ms + 20U, &event) == 1U);
  return event;
}

int main(void)
{
  NetworkAlertLogic logic;
  NetworkAlertEvent event;
  NetworkAlertVitals normal = MakeVitals(74U, 98U, 368, "Normal");
  NetworkAlertVitals low_spo2 = MakeVitals(78U, 87U, 369, "Low_SpO2");
  char json[512];
  size_t json_length;

  NetworkAlertLogic_Init(&logic);
  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 100U, &event) == 0U);
  assert(logic.active == 0U);

  event = Trigger(&logic, &low_spo2, 380U);
  assert(event.type == NETWORK_ALERT_EVENT_TRIGGERED);
  assert(event.severity == NETWORK_ALERT_SEVERITY_CRITICAL);
  assert(event.reasons == NETWORK_ALERT_REASON_SPO2_LOW);
  assert(event.sequence == 1U);

  json_length = NetworkAlertLogic_FormatJson(&event, json, sizeof(json));
  assert(json_length > 0U);
  assert(strstr(json, "\"event\":\"triggered\"") != NULL);
  assert(strstr(json, "\"reasons\":[\"low_spo2\"]") != NULL);
  assert(strstr(json, "\"temperature_c\":36.9") != NULL);

  assert(NetworkAlertLogic_Evaluate(&logic, &low_spo2,
                                    400U + NETWORK_ALERT_REPEAT_MS - 1U,
                                    &event) == 0U);
  assert(NetworkAlertLogic_Evaluate(&logic, &low_spo2,
                                    400U + NETWORK_ALERT_REPEAT_MS,
                                    &event) == 1U);
  assert(event.type == NETWORK_ALERT_EVENT_REMINDER);
  assert(event.sequence == 2U);

  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 31000U, &event) == 0U);
  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 31100U, &event) == 0U);
  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 31200U, &event) == 1U);
  assert(event.type == NETWORK_ALERT_EVENT_RECOVERED);
  assert(event.severity == NETWORK_ALERT_SEVERITY_NORMAL);
  assert(event.reasons == 0U);
  assert(logic.active == 0U);

  {
    const struct ThresholdCase
    {
      NetworkAlertVitals vitals;
      uint8_t expected_reason;
    } cases[] = {
      {MakeVitals(59U, 98U, 368, "Bradycardia"), NETWORK_ALERT_REASON_HR_LOW},
      {MakeVitals(101U, 98U, 368, "Tachycardia"), NETWORK_ALERT_REASON_HR_HIGH},
      {MakeVitals(74U, 98U, 349, "Abnormal_Temp"), NETWORK_ALERT_REASON_TEMP_LOW},
      {MakeVitals(74U, 98U, 380, "Abnormal_Temp"), NETWORK_ALERT_REASON_TEMP_HIGH}
    };
    size_t index;

    for (index = 0U; index < (sizeof(cases) / sizeof(cases[0])); index++)
    {
      NetworkAlertLogic_Init(&logic);
      event = Trigger(&logic, &cases[index].vitals, (uint32_t)(40000U + index * 100U));
      assert(event.reasons == cases[index].expected_reason);
      assert(event.severity == NETWORK_ALERT_SEVERITY_WARNING);
    }
  }

  NetworkAlertLogic_Init(&logic);
  normal = MakeVitals(60U, 92U, 350, "Normal");
  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 50000U, &event) == 0U);
  normal = MakeVitals(100U, 100U, 379, "Normal");
  assert(NetworkAlertLogic_Evaluate(&logic, &normal, 50100U, &event) == 0U);

  puts("network alert logic tests passed");
  return 0;
}
