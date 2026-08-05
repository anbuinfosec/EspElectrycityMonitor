#include "settings.h"
#include "storage.h"
#include <ArduinoJson.h>

Settings currentSettings;

void loadSettings() {
    String jsonStr = readFile("/settings.json");
    if (jsonStr.isEmpty()) {
        factoryReset();
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        factoryReset();
        return;
    }
    
    currentSettings.deviceName = doc["deviceName"] | "ESP8266 Monitor";
    currentSettings.wifiSSID = doc["wifiSSID"] | "";
    currentSettings.wifiPassword = doc["wifiPassword"] | "";
    currentSettings.apSSID = doc["apSSID"] | "EspElectrycityMonitor";
    currentSettings.apPassword = doc["apPassword"] | "anbuinfosec";
    currentSettings.ntpServer = doc["ntpServer"] | "pool.ntp.org";
    currentSettings.timezoneOffset = doc["timezoneOffset"] | 0;
    currentSettings.telegramEnabled = doc["telegramEnabled"] | false;
    currentSettings.telegramBotToken = doc["telegramBotToken"] | "";
    currentSettings.telegramChatId = doc["telegramChatId"] | "";
    currentSettings.buzzerEnabled = doc["buzzerEnabled"] | false;
    currentSettings.buzzerPin = doc["buzzerPin"] | -1;
    currentSettings.adminPassword = doc["adminPassword"] | "admin";
    currentSettings.use24hFormat = doc["use24hFormat"] | true;
}

void saveSettings() {
    JsonDocument doc;
    doc["deviceName"] = currentSettings.deviceName;
    doc["wifiSSID"] = currentSettings.wifiSSID;
    doc["wifiPassword"] = currentSettings.wifiPassword;
    doc["apSSID"] = currentSettings.apSSID;
    doc["apPassword"] = currentSettings.apPassword;
    doc["ntpServer"] = currentSettings.ntpServer;
    doc["timezoneOffset"] = currentSettings.timezoneOffset;
    doc["telegramEnabled"] = currentSettings.telegramEnabled;
    doc["telegramBotToken"] = currentSettings.telegramBotToken;
    doc["telegramChatId"] = currentSettings.telegramChatId;
    doc["buzzerEnabled"] = currentSettings.buzzerEnabled;
    doc["buzzerPin"] = currentSettings.buzzerPin;
    doc["adminPassword"] = currentSettings.adminPassword;
    doc["use24hFormat"] = currentSettings.use24hFormat;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    writeFile("/settings.json", jsonStr.c_str());
}

void factoryReset() {
    currentSettings.deviceName = "ESP8266 Monitor";
    currentSettings.wifiSSID = "";
    currentSettings.wifiPassword = "";
    currentSettings.apSSID = "EspElectrycityMonitor";
    currentSettings.apPassword = "anbuinfosec";
    currentSettings.ntpServer = "pool.ntp.org";
    currentSettings.timezoneOffset = 0;
    currentSettings.telegramEnabled = false;
    currentSettings.telegramBotToken = "";
    currentSettings.telegramChatId = "";
    currentSettings.buzzerEnabled = false;
    currentSettings.buzzerPin = -1;
    currentSettings.adminPassword = "admin";
    currentSettings.use24hFormat = true;
    saveSettings();
}
