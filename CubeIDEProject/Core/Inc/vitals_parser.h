#ifndef VITALS_PARSER_H
#define VITALS_PARSER_H

#include <stdint.h>

typedef struct
{
  uint16_t heart_rate;
  uint8_t spo2;
  int16_t temperature_tenths;
  int16_t ecg_millivolts;
  char scenario[24];
} VitalsFrame;

uint8_t VitalsParser_Parse(const char *json, VitalsFrame *frame);

#endif /* VITALS_PARSER_H */
