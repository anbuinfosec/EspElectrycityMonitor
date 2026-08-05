#pragma once
#include <Arduino.h>

bool isPowerLossReset();
String formatDuration(uint32_t seconds);
String formatDateTime(time_t epochTime);
time_t getEpochTime();
void setEpochTime(time_t epoch);
