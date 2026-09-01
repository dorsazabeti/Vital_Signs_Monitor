#include "vitals_parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
  VitalsFrame frame;

  assert(VitalsParser_Parse(
      "{\"scenario\":\"Normal\",\"hr\":74,\"spo2\":98,\"temp\":36.8,\"ecg\":0.123}",
      &frame) == 1U);
  assert(frame.heart_rate == 74U);
  assert(frame.spo2 == 98U);
  assert(frame.temperature_tenths == 368);
  assert(frame.ecg_millivolts == 123);
  assert(strcmp(frame.scenario, "Normal") == 0);

  assert(VitalsParser_Parse(
      "{\"ecg\":-0.025,\"temp\":37,\"spo2\":89,\"hr\":105,\"scenario\":\"Low_SpO2\"}",
      &frame) == 1U);
  assert(frame.temperature_tenths == 370);
  assert(frame.ecg_millivolts == -25);

  assert(VitalsParser_Parse(
      "{\"scenario\":\"Normal\",\"hr\":29,\"spo2\":98,\"temp\":36.8,\"ecg\":0.1}",
      &frame) == 0U);
  assert(VitalsParser_Parse(
      "{\"scenario\":\"Normal\",\"hr\":74x,\"spo2\":98,\"temp\":36.8,\"ecg\":0.1}",
      &frame) == 0U);
  assert(VitalsParser_Parse(
      "{\"scenario\":\"Normal\",\"hr\":74,\"spo2\":98,\"temp\":36.8}",
      &frame) == 0U);
  assert(VitalsParser_Parse("not-json", &frame) == 0U);
  assert(VitalsParser_Parse(NULL, &frame) == 0U);

  puts("vitals parser tests passed");
  return 0;
}
