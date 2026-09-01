#ifndef NETWORK_ALERT_LOGIC_H
#define NETWORK_ALERT_LOGIC_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
  NETWORK_ALERT_EVENT_NONE = 0,
  NETWORK_ALERT_EVENT_TRIGGERED,
  NETWORK_ALERT_EVENT_REMINDER,
  NETWORK_ALERT_EVENT_RECOVERED
} NetworkAlertEventType;

typedef enum
{
  NETWORK_ALERT_SEVERITY_NORMAL = 0,
  NETWORK_ALERT_SEVERITY_WARNING,
  NETWORK_ALERT_SEVERITY_CRITICAL
} NetworkAlertSeverity;

enum
{
  NETWORK_ALERT_REASON_HR_LOW = (1U << 0),
  NETWORK_ALERT_REASON_HR_HIGH = (1U << 1),
  NETWORK_ALERT_REASON_SPO2_LOW = (1U << 2),
  NETWORK_ALERT_REASON_TEMP_LOW = (1U << 3),
  NETWORK_ALERT_REASON_TEMP_HIGH = (1U << 4)
};

typedef struct
{
  uint16_t heart_rate;
  uint8_t spo2;
  int16_t temperature_tenths;
  char scenario[24];
} NetworkAlertVitals;

typedef struct
{
  NetworkAlertEventType type;
  NetworkAlertSeverity severity;
  uint8_t reasons;
  uint32_t sequence;
  uint32_t uptime_ms;
  NetworkAlertVitals vitals;
} NetworkAlertEvent;

typedef struct
{
  uint8_t active;
  uint8_t abnormal_samples;
  uint8_t normal_samples;
  uint8_t active_reasons;
  uint32_t last_event_ms;
  uint32_t next_sequence;
} NetworkAlertLogic;

void NetworkAlertLogic_Init(NetworkAlertLogic *logic);
uint8_t NetworkAlertLogic_Evaluate(NetworkAlertLogic *logic,
                                   const NetworkAlertVitals *vitals,
                                   uint32_t now_ms,
                                   NetworkAlertEvent *event);
size_t NetworkAlertLogic_FormatJson(const NetworkAlertEvent *event,
                                    char *output,
                                    size_t output_size);

#endif /* NETWORK_ALERT_LOGIC_H */
