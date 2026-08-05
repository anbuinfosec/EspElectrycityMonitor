#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

enum EventType {
    EVENT_AVAILABLE = 0,
    EVENT_LOST = 1,
    EVENT_RESTART = 2,
    EVENT_SYSTEM = 3
};

void logEvent(EventType type, uint32_t duration, const String& description, time_t timestamp = 0);
void updateLastAlive();
void processBootEvent();
void clearHistory();
void rotateLogs();
void rotateLogsWithLimit(time_t retentionLimit);
String getEventsJson(int page = 1, int limit = 50);
int getTotalEvents();
