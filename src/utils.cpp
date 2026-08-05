#include "utils.h"
#include <sys/time.h>
#include <sys/reent.h>
#include <errno.h>
#ifdef ESP32
#include <esp_system.h>
bool isPowerLossReset() {
    esp_reset_reason_t reason = esp_reset_reason();
    return (reason == ESP_RST_POWERON || reason == ESP_RST_EXT);
}
#else
#include <user_interface.h>
bool isPowerLossReset() {
    rst_info *resetInfo = ESP.getResetInfoPtr();
    // REASON_DEFAULT_RST is normal power on
    // REASON_EXT_SYS_RST is external reset (often power cycle)
    if (resetInfo->reason == REASON_DEFAULT_RST || resetInfo->reason == REASON_EXT_SYS_RST) {
        return true; 
    }
    return false;
}
#endif

String formatDuration(uint32_t seconds) {
    if (seconds == 0) return "0 Seconds";
    
    uint32_t h = seconds / 3600;
    uint32_t m = (seconds % 3600) / 60;
    uint32_t s = seconds % 60;
    
    String res = "";
    if (h > 0) res += String(h) + " Hour" + (h > 1 ? "s " : " ");
    if (m > 0) res += String(m) + " Minute" + (m > 1 ? "s " : " ");
    if (s > 0 || (h == 0 && m == 0)) res += String(s) + " Second" + (s != 1 ? "s" : "");
    
    res.trim();
    return res;
}

String formatDateTime(time_t epochTime) {
    struct tm * ti = localtime(&epochTime);
    if (!ti) {
        return String("2026-01-01 00:00:00");
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
            ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
            ti->tm_hour, ti->tm_min, ti->tm_sec);
    return String(buffer);
}

static uint64_t epochTimeOffsetUs = 0;

time_t getEpochTime() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec;
}

void setEpochTime(time_t epoch) {
    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
}

#ifndef ESP32
extern "C" {
    int _gettimeofday_r(struct _reent* unused, struct timeval *tp, void *tzp) __attribute__((weak));
    time_t time(time_t *t) __attribute__((weak));
    int settimeofday(const struct timeval* tv, const struct timezone* tz) __attribute__((weak));
}

extern "C" int _gettimeofday_r(struct _reent* unused, struct timeval *tp, void *tzp) {
    (void) unused;
    (void) tzp;

    if (tp) {
        uint64_t now_us = micros64() + epochTimeOffsetUs;
        tp->tv_sec = now_us / 1000000ULL;
        tp->tv_usec = now_us % 1000000ULL;
    }
    return 0;
}

extern "C" time_t time(time_t *t) {
    time_t currentTime = (micros64() + epochTimeOffsetUs) / 1000000ULL;
    if (t) {
        *t = currentTime;
    }
    return currentTime;
}

extern "C" int settimeofday(const struct timeval* tv, const struct timezone* tz) {
    bool fromSntp = false;
    if (tz == (struct timezone*)0xFeedC0de) {
        tz = nullptr;
        fromSntp = true;
    }

    if (tz || !tv) {
        return EINVAL;
    }

    uint64_t currentUs = micros64();
    epochTimeOffsetUs = (uint64_t)tv->tv_sec * 1000000ULL + tv->tv_usec - currentUs;
    (void)fromSntp;
    return 0;
}
#endif
