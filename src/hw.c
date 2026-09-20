#include "hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int hw_get_battery_percent(void) {
    const char *paths[] = {
        "/sys/class/power_supply/battery/capacity",
        "/sys/class/power_supply/axp20x-battery/capacity",
        "/sys/class/power_supply/axp209-battery/capacity",
        NULL
    };

    for (int i = 0; paths[i] != NULL; ++i) {
        FILE *f = fopen(paths[i], "r");
        if (f) {
            int cap = -1;
            if (fscanf(f, "%d", &cap) == 1) {
                fclose(f);
                if (cap < 0) cap = 0;
                if (cap > 100) cap = 100;
                return cap;
            }
            fclose(f);
        }
    }

    /* Fallback default for PC testing */
    return 85;
}

int hw_is_charging(void) {
    const char *paths[] = {
        "/sys/class/power_supply/battery/status",
        "/sys/class/power_supply/axp20x-battery/status",
        NULL
    };

    char status[32];
    for (int i = 0; paths[i] != NULL; ++i) {
        FILE *f = fopen(paths[i], "r");
        if (f) {
            if (fgets(status, sizeof(status), f)) {
                fclose(f);
                if (strncmp(status, "Charging", 8) == 0) {
                    return 1;
                }
                return 0;
            }
            fclose(f);
        }
    }
    return 0;
}

void hw_get_time_str(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return;

    time_t rawtime;
    time(&rawtime);
    struct tm *info = localtime(&rawtime);

    if (info) {
        snprintf(buf, max_len, "%02d:%02d", info->tm_hour, info->tm_min);
    } else {
        snprintf(buf, max_len, "12:00");
    }
}
