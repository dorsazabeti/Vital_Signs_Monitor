#include "network_alert_logic.h"
#include "network_alert_config.h"

#include <stdio.h>
#include <string.h>

static uint8_t NetworkAlertLogic_Reasons(const NetworkAlertVitals *vitals)
{
  uint8_t reasons = 0U;

  if (vitals->heart_rate < NETWORK_ALERT_HR_LOW)
  {
    reasons |= NETWORK_ALERT_REASON_HR_LOW;
  }
  else if (vitals->heart_rate > NETWORK_ALERT_HR_HIGH)
  {
    reasons |= NETWORK_ALERT_REASON_HR_HIGH;
  }

  if (vitals->spo2 < NETWORK_ALERT_SPO2_LOW)
  {
    reasons |= NETWORK_ALERT_REASON_SPO2_LOW;
  }

  if (vitals->temperature_tenths < NETWORK_ALERT_TEMP_LOW_TENTHS)
  {
    reasons |= NETWORK_ALERT_REASON_TEMP_LOW;
  }
  else if (vitals->temperature_tenths >= NETWORK_ALERT_TEMP_HIGH_TENTHS)
  {
    reasons |= NETWORK_ALERT_REASON_TEMP_HIGH;
  }

  return reasons;
}

static NetworkAlertSeverity NetworkAlertLogic_Severity(const NetworkAlertVitals *vitals,
                                                        uint8_t reasons)
{
  if (reasons == 0U)
  {
    return NETWORK_ALERT_SEVERITY_NORMAL;
  }

  if ((vitals->heart_rate < VITALS_HR_CRITICAL_LOW) ||
      (vitals->heart_rate > VITALS_HR_CRITICAL_HIGH) ||
      (vitals->spo2 < VITALS_SPO2_CRITICAL_LOW) ||
      (vitals->temperature_tenths < VITALS_TEMP_CRITICAL_LOW) ||
      (vitals->temperature_tenths >= VITALS_TEMP_CRITICAL_HIGH))
  {
    return NETWORK_ALERT_SEVERITY_CRITICAL;
  }

  return NETWORK_ALERT_SEVERITY_WARNING;
}

static void NetworkAlertLogic_CopyVitals(NetworkAlertVitals *destination,
                                         const NetworkAlertVitals *source)
{
  *destination = *source;
  destination->scenario[sizeof(destination->scenario) - 1U] = '\0';
}

static void NetworkAlertLogic_MakeEvent(NetworkAlertLogic *logic,
                                        const NetworkAlertVitals *vitals,
                                        uint32_t now_ms,
                                        NetworkAlertEventType type,
                                        uint8_t reasons,
                                        NetworkAlertEvent *event)
{
  memset(event, 0, sizeof(*event));
  event->type = type;
  event->severity = NetworkAlertLogic_Severity(vitals, reasons);
  event->reasons = reasons;
  event->sequence = logic->next_sequence++;
  event->uptime_ms = now_ms;
  NetworkAlertLogic_CopyVitals(&event->vitals, vitals);
  logic->last_event_ms = now_ms;
}

void NetworkAlertLogic_Init(NetworkAlertLogic *logic)
{
  if (logic != NULL)
  {
    memset(logic, 0, sizeof(*logic));
    logic->next_sequence = 1U;
  }
}

uint8_t NetworkAlertLogic_Evaluate(NetworkAlertLogic *logic,
                                   const NetworkAlertVitals *vitals,
                                   uint32_t now_ms,
                                   NetworkAlertEvent *event)
{
  uint8_t reasons;

  if ((logic == NULL) || (vitals == NULL) || (event == NULL))
  {
    return 0U;
  }

  reasons = NetworkAlertLogic_Reasons(vitals);

  if (reasons != 0U)
  {
    logic->normal_samples = 0U;
    if (logic->abnormal_samples < 255U)
    {
      logic->abnormal_samples++;
    }

    if ((logic->active == 0U) &&
        (logic->abnormal_samples >= NETWORK_ALERT_CONFIRM_SAMPLES))
    {
      logic->active = 1U;
      logic->active_reasons = reasons;
      NetworkAlertLogic_MakeEvent(logic, vitals, now_ms,
                                  NETWORK_ALERT_EVENT_TRIGGERED, reasons, event);
      return 1U;
    }

    if (logic->active != 0U)
    {
      logic->active_reasons = reasons;
      if ((uint32_t)(now_ms - logic->last_event_ms) >= NETWORK_ALERT_REPEAT_MS)
      {
        NetworkAlertLogic_MakeEvent(logic, vitals, now_ms,
                                    NETWORK_ALERT_EVENT_REMINDER, reasons, event);
        return 1U;
      }
    }
  }
  else
  {
    logic->abnormal_samples = 0U;
    if (logic->normal_samples < 255U)
    {
      logic->normal_samples++;
    }

    if ((logic->active != 0U) &&
        (logic->normal_samples >= NETWORK_ALERT_CLEAR_SAMPLES))
    {
      logic->active = 0U;
      logic->active_reasons = 0U;
      NetworkAlertLogic_MakeEvent(logic, vitals, now_ms,
                                  NETWORK_ALERT_EVENT_RECOVERED, 0U, event);
      return 1U;
    }
  }

  return 0U;
}

static const char *NetworkAlertLogic_EventName(NetworkAlertEventType type)
{
  switch (type)
  {
    case NETWORK_ALERT_EVENT_TRIGGERED:
      return "triggered";
    case NETWORK_ALERT_EVENT_REMINDER:
      return "reminder";
    case NETWORK_ALERT_EVENT_RECOVERED:
      return "recovered";
    default:
      return "none";
  }
}

static const char *NetworkAlertLogic_SeverityName(NetworkAlertSeverity severity)
{
  switch (severity)
  {
    case NETWORK_ALERT_SEVERITY_CRITICAL:
      return "critical";
    case NETWORK_ALERT_SEVERITY_WARNING:
      return "warning";
    default:
      return "normal";
  }
}

static size_t NetworkAlertLogic_Append(char *output, size_t output_size,
                                       size_t used, const char *text)
{
  size_t length = strlen(text);

  if ((used >= output_size) || (length >= (output_size - used)))
  {
    return output_size;
  }

  memcpy(output + used, text, length);
  output[used + length] = '\0';
  return used + length;
}

static size_t NetworkAlertLogic_AppendReasons(char *output, size_t output_size,
                                              size_t used, uint8_t reasons)
{
  struct ReasonName
  {
    uint8_t flag;
    const char *name;
  };
  static const struct ReasonName reason_names[] = {
    {NETWORK_ALERT_REASON_HR_LOW, "low_heart_rate"},
    {NETWORK_ALERT_REASON_HR_HIGH, "high_heart_rate"},
    {NETWORK_ALERT_REASON_SPO2_LOW, "low_spo2"},
    {NETWORK_ALERT_REASON_TEMP_LOW, "low_temperature"},
    {NETWORK_ALERT_REASON_TEMP_HIGH, "high_temperature"}
  };
  uint8_t first = 1U;
  size_t index;

  used = NetworkAlertLogic_Append(output, output_size, used, "[");
  for (index = 0U; index < (sizeof(reason_names) / sizeof(reason_names[0])); index++)
  {
    if ((reasons & reason_names[index].flag) != 0U)
    {
      if (first == 0U)
      {
        used = NetworkAlertLogic_Append(output, output_size, used, ",");
      }
      used = NetworkAlertLogic_Append(output, output_size, used, "\"");
      used = NetworkAlertLogic_Append(output, output_size, used, reason_names[index].name);
      used = NetworkAlertLogic_Append(output, output_size, used, "\"");
      first = 0U;
    }
  }
  return NetworkAlertLogic_Append(output, output_size, used, "]");
}

static void NetworkAlertLogic_SanitizeScenario(const char *input, char *output,
                                               size_t output_size)
{
  size_t index = 0U;

  if (output_size == 0U)
  {
    return;
  }

  while ((input[index] != '\0') && (index < (output_size - 1U)))
  {
    char value = input[index];
    if (((value >= 'a') && (value <= 'z')) ||
        ((value >= 'A') && (value <= 'Z')) ||
        ((value >= '0') && (value <= '9')) ||
        (value == '_') || (value == '-') || (value == ' '))
    {
      output[index] = value;
    }
    else
    {
      output[index] = '_';
    }
    index++;
  }
  output[index] = '\0';
}

size_t NetworkAlertLogic_FormatJson(const NetworkAlertEvent *event,
                                    char *output,
                                    size_t output_size)
{
  char scenario[sizeof(event->vitals.scenario)];
  int written;
  int temperature_absolute;
  const char *temperature_sign;
  size_t used;

  if ((event == NULL) || (output == NULL) || (output_size == 0U))
  {
    return 0U;
  }

  NetworkAlertLogic_SanitizeScenario(event->vitals.scenario, scenario, sizeof(scenario));
  temperature_sign = (event->vitals.temperature_tenths < 0) ? "-" : "";
  temperature_absolute = event->vitals.temperature_tenths;
  if (temperature_absolute < 0)
  {
    temperature_absolute = -temperature_absolute;
  }

  written = snprintf(output, output_size,
                     "{\"version\":1,\"type\":\"vital_alert\","
                     "\"event\":\"%s\",\"sequence\":%lu,\"uptime_ms\":%lu,"
                     "\"severity\":\"%s\",\"scenario\":\"%s\","
                     "\"heart_rate\":%u,\"spo2\":%u,\"temperature_c\":%s%d.%d,"
                     "\"reasons\":",
                     NetworkAlertLogic_EventName(event->type),
                     (unsigned long)event->sequence,
                     (unsigned long)event->uptime_ms,
                     NetworkAlertLogic_SeverityName(event->severity),
                     scenario,
                     (unsigned int)event->vitals.heart_rate,
                     (unsigned int)event->vitals.spo2,
                     temperature_sign,
                     temperature_absolute / 10,
                     temperature_absolute % 10);

  if ((written < 0) || ((size_t)written >= output_size))
  {
    output[0] = '\0';
    return 0U;
  }

  used = NetworkAlertLogic_AppendReasons(output, output_size, (size_t)written,
                                         event->reasons);
  used = NetworkAlertLogic_Append(output, output_size, used, "}");
  if (used >= output_size)
  {
    output[0] = '\0';
    return 0U;
  }

  return used;
}
