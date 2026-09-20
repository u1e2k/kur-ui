#ifndef HW_H
#define HW_H

#include <stddef.h>

/* Get battery percentage (0-100), or -1 if not available */
int hw_get_battery_percent(void);

/* Check if battery is currently charging (1 = charging, 0 = discharging/not available) */
int hw_is_charging(void);

/* Get current local time formatted as "HH:MM" */
void hw_get_time_str(char *buf, size_t max_len);

#endif /* HW_H */
