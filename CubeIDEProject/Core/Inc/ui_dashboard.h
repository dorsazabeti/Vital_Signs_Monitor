#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include "lvgl.h"

#include <stdint.h>

void UI_Dashboard_Init(lv_obj_t *screen);
void Vitals_UpdateUI(uint16_t heart_rate, uint8_t spo2,
                     int16_t temperature_tenths, const char *scenario);
void Vitals_SetDataAvailable(uint8_t available);
void ECG_Update(int16_t ecg_millivolts);

#endif /* UI_DASHBOARD_H */
