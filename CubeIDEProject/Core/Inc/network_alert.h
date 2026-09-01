#ifndef NETWORK_ALERT_H
#define NETWORK_ALERT_H

#include <stdint.h>

typedef enum
{
  NETWORK_ALERT_STATE_OFF = 0,
  NETWORK_ALERT_STATE_INITIALIZING,
  NETWORK_ALERT_STATE_LINK_DOWN,
  NETWORK_ALERT_STATE_WAITING_FOR_ARP,
  NETWORK_ALERT_STATE_READY,
  NETWORK_ALERT_STATE_ERROR
} NetworkAlertState;

typedef struct
{
  NetworkAlertState state;
  uint8_t link_up;
  uint8_t alert_active;
  uint32_t frames_received;
  uint32_t alerts_sent;
  uint32_t transmit_errors;
  uint32_t receive_errors;
} NetworkAlertStatus;

/* Initialize the Ethernet alert service. Safe to call once from a FreeRTOS task. */
void NetworkAlert_Init(void);

/* Non-blocking service routine. Call regularly (the default task calls it). */
void NetworkAlert_Process(void);

/* Feed each valid vital-sign sample to the alert detector. */
void NetworkAlert_UpdateVitals(uint16_t heart_rate, uint8_t spo2,
                               int16_t temperature_tenths,
                               const char *scenario);

NetworkAlertStatus NetworkAlert_GetStatus(void);

#endif /* NETWORK_ALERT_H */
