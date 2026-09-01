#ifndef VITALS_THRESHOLDS_H
#define VITALS_THRESHOLDS_H

/* Shared limits used by both the on-screen warning and Ethernet alert logic. */
#define VITALS_HR_LOW                   60U
#define VITALS_HR_HIGH                  100U
#define VITALS_SPO2_LOW                 92U
#define VITALS_TEMP_LOW_TENTHS          350
#define VITALS_TEMP_HIGH_TENTHS         380

#define VITALS_HR_CRITICAL_LOW          45U
#define VITALS_HR_CRITICAL_HIGH         130U
#define VITALS_SPO2_CRITICAL_LOW        88U
#define VITALS_TEMP_CRITICAL_LOW        340
#define VITALS_TEMP_CRITICAL_HIGH       395

#endif /* VITALS_THRESHOLDS_H */
