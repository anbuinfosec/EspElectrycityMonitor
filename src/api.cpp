#include "api.h"
#include "settings.h"
#include "history.h"
#include "reports.h"
#include "utils.h"
#include "telegram.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <AsyncJson.h>

void setupAPI(AsyncWebServer& server) {
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        String json = "{\"status\":\"online\",\"time\":" + String(getEpochTime()) + "}";
        request->send(200, "application/json", json);
    });

    server.on("/api/events", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        int page = 1;
        int limit = 50;
        if(request->hasParam("page")) page = request->getParam("page")->value().toInt();
        if(request->hasParam("limit")) limit = request->getParam("limit")->value().toInt();
        
        String json = getEventsJson(page, limit);
        request->send(200, "application/json", json);
    });

    server.on("/api/daily", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        request->send(200, "application/json", getDailyReportJson());
    });

    server.on("/api/weekly", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        request->send(200, "application/json", getWeeklyReportJson());
    });

    server.on("/api/monthly", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        request->send(200, "application/json", getMonthlyReportJson());
    });

    server.on("/api/statistics", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        request->send(200, "application/json", getStatisticsJson());
    });

    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        JsonDocument doc;
        doc["deviceName"] = currentSettings.deviceName;
        doc["wifiSSID"] = currentSettings.wifiSSID;
        doc["apSSID"] = currentSettings.apSSID;
        doc["ntpServer"] = currentSettings.ntpServer;
        doc["timezoneOffset"] = currentSettings.timezoneOffset;
        doc["telegramEnabled"] = currentSettings.telegramEnabled;
        doc["telegramBotToken"] = currentSettings.telegramBotToken;
        doc["telegramChatId"] = currentSettings.telegramChatId;
        doc["buzzerEnabled"] = currentSettings.buzzerEnabled;
        doc["buzzerPin"] = currentSettings.buzzerPin;
        doc["use24hFormat"] = currentSettings.use24hFormat;
        String jsonStr;
        serializeJson(doc, jsonStr);
        request->send(200, "application/json", jsonStr);
    });

    server.on("/api/wifi-scan", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();

        int n = WiFi.scanComplete();
        if(n == -2) {
            WiFi.mode(WIFI_AP_STA);
            WiFi.scanNetworks(true);
            request->send(202, "application/json", "{\"status\":\"scanning\"}");
        } else if(n == -1) {
            request->send(202, "application/json", "{\"status\":\"scanning\"}");
        } else if(n >= 0) {
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();

            for (int i = 0; i < n; i++) {
                JsonObject obj = arr.add<JsonObject>();
                obj["ssid"] = WiFi.SSID(i);
                obj["rssi"] = WiFi.RSSI(i);
                obj["secure"] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
            }
            
            WiFi.scanDelete();
            String jsonStr;
            serializeJson(doc, jsonStr);
            request->send(200, "application/json", jsonStr);
        } else {
            WiFi.mode(WIFI_AP_STA);
            WiFi.scanNetworks(true);
            request->send(202, "application/json", "{\"status\":\"scanning\"}");
        }
    });

    AsyncCallbackJsonWebHandler* wifiConnectHandler = new AsyncCallbackJsonWebHandler("/api/wifi-connect", [](AsyncWebServerRequest *request, JsonVariant &json) {
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        
        JsonObject payload = json.as<JsonObject>();
        const char* ssid = payload["ssid"] | "";
        const char* password = payload["password"] | "";
        
        if (strlen(ssid) == 0) {
            request->send(400, "application/json", "{\"message\":\"SSID required\"}");
            return;
        }

        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(ssid, password);

        request->send(200, "application/json", "{\"status\":\"connecting\"}");
    });
    server.addHandler(wifiConnectHandler);

    server.on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        
        int status = WiFi.status();
        JsonDocument doc;
        if (status == WL_CONNECTED) {
            doc["status"] = "connected";
            doc["ssid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            doc["rssi"] = WiFi.RSSI();
        } else if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED || status == WL_WRONG_PASSWORD) {
            doc["status"] = "failed";
        } else {
            doc["status"] = "disconnected";
        }
        String jsonStr;
        serializeJson(doc, jsonStr);
        request->send(200, "application/json", jsonStr);
    });

    server.on("/api/wifi-forget", HTTP_POST, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        
        WiFi.disconnect(true);
        currentSettings.wifiSSID = "";
        currentSettings.wifiPassword = "";
        saveSettings();
        
        request->send(200, "application/json", "{\"status\":\"forgotten\"}");
    });

    AsyncCallbackJsonWebHandler* settingsHandler = new AsyncCallbackJsonWebHandler("/api/settings", [](AsyncWebServerRequest *request, JsonVariant &json) {
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        JsonObject obj = json.as<JsonObject>();
        if (!obj["deviceName"].isNull()) currentSettings.deviceName = obj["deviceName"].as<String>();
        if (!obj["wifiSSID"].isNull()) currentSettings.wifiSSID = obj["wifiSSID"].as<String>();
        if (!obj["wifiPassword"].isNull()) currentSettings.wifiPassword = obj["wifiPassword"].as<String>();
        if (!obj["apSSID"].isNull()) currentSettings.apSSID = obj["apSSID"].as<String>();
        if (!obj["apPassword"].isNull()) currentSettings.apPassword = obj["apPassword"].as<String>();
        if (!obj["ntpServer"].isNull()) currentSettings.ntpServer = obj["ntpServer"].as<String>();
        if (!obj["timezoneOffset"].isNull()) currentSettings.timezoneOffset = obj["timezoneOffset"].as<int>();
        if (!obj["telegramEnabled"].isNull()) currentSettings.telegramEnabled = obj["telegramEnabled"].as<bool>();
        if (!obj["telegramBotToken"].isNull()) currentSettings.telegramBotToken = obj["telegramBotToken"].as<String>();
        if (!obj["telegramChatId"].isNull()) currentSettings.telegramChatId = obj["telegramChatId"].as<String>();
        if (!obj["buzzerEnabled"].isNull()) currentSettings.buzzerEnabled = obj["buzzerEnabled"].as<bool>();
        if (!obj["buzzerPin"].isNull()) currentSettings.buzzerPin = obj["buzzerPin"].as<int>();
        if (!obj["adminPassword"].isNull()) {
            String newPwd = obj["adminPassword"].as<String>();
            if(newPwd != "") currentSettings.adminPassword = newPwd;
        }
        if (!obj["use24hFormat"].isNull()) currentSettings.use24hFormat = obj["use24hFormat"].as<bool>();
        
        if (currentSettings.telegramEnabled && currentSettings.telegramBotToken != "" && currentSettings.telegramChatId != "") {
            extern bool requestTelegramTest;
            requestTelegramTest = true;
        }
        
        if (currentSettings.buzzerEnabled) {
            if (currentSettings.buzzerPin < 0 || currentSettings.buzzerPin > 16) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid Buzzer GPIO Pin.\"}");
                return;
            }
            pinMode(currentSettings.buzzerPin, OUTPUT);
            digitalWrite(currentSettings.buzzerPin, HIGH);
            delay(100);
            digitalWrite(currentSettings.buzzerPin, LOW);
        }

        saveSettings();
        request->send(200, "application/json", "{\"status\":\"saved\"}");
    });
    server.addHandler(settingsHandler);

    server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"restarting\"}");
        delay(1000);
        ESP.restart();
    });

    server.on("/api/clear-history", HTTP_POST, [](AsyncWebServerRequest *request){
        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) return request->requestAuthentication();
        clearHistory();
        request->send(200, "application/json", "{\"status\":\"cleared\"}");
    });
}
