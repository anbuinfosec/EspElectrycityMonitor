#pragma once
#include <Arduino.h>

struct Settings {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String apSSID;
    String apPassword;
    String ntpServer;
    int timezoneOffset;
    bool telegramEnabled;
    String telegramBotToken;
    String telegramChatId;
    bool buzzerEnabled;
    int buzzerPin;
    String adminPassword;
    bool use24hFormat;
};

extern Settings currentSettings;

void loadSettings();
void saveSettings();
void factoryReset();
