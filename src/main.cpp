#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "storage.h"
#include "settings.h"
#include "history.h"
#include "webserver.h"
#include "utils.h"
#include "telegram.h"
#include <ElegantOTA.h>

WiFiUDP ntpUDP;
NTPClient *timeClient;

unsigned long lastAliveUpdate = 0;
unsigned long lastLogRotation = 0;
bool timeSynced = false;

#include <lwip/napt.h>
#include <lwip/dns.h>

bool requestTelegramTest = false;

void setupWiFi() {
    // Always keep AP on so users can connect
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(currentSettings.apSSID.c_str(), currentSettings.apPassword.c_str());
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    if (currentSettings.wifiSSID != "") {
        WiFi.begin(currentSettings.wifiSSID.c_str(), currentSettings.wifiPassword.c_str());
        
        Serial.print("Connecting to WiFi");
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println(WiFi.localIP());

        // Enable NAT routing (128 entries is enough for home use, saves ~28KB RAM)
        err_t err = ip_napt_init(128, 10);
        if (err == ERR_OK) {
            err = ip_napt_enable_no(SOFTAP_IF, 1);
            if (err == ERR_OK) {
                Serial.println("NAT enabled successfully (Internet Sharing)");
            }
        }
        
        // Pass the router's DNS to the softAP DHCP clients
        auto& server = WiFi.softAPDhcpServer();
        server.setDns(WiFi.dnsIP(0));
    } else {
        Serial.println("WiFi STA connection failed or not configured.");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting PowerMon...");

    initStorage();
    loadSettings();
    setupWiFi();
    
    timeClient = new NTPClient(ntpUDP, currentSettings.ntpServer.c_str(), currentSettings.timezoneOffset * 3600, 60000);
    timeClient->begin();

    initWebServer();
    
    // In ESPAsyncWebServer, server is extern AsyncWebServer server; we can just pass &server
    // Wait, server is defined in webserver.cpp, but I can include it or declare it
    // Actually, AsyncElegantOTA.begin(&server) needs the server object.
    // I should expose `server` from webserver.h
    extern AsyncWebServer server;
    ElegantOTA.begin(&server);
    ElegantOTA.setAuth("admin", currentSettings.adminPassword.c_str());

    if (currentSettings.buzzerEnabled && currentSettings.buzzerPin >= 0) {
        pinMode(currentSettings.buzzerPin, OUTPUT);
        digitalWrite(currentSettings.buzzerPin, HIGH);
        delay(200);
        digitalWrite(currentSettings.buzzerPin, LOW);
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        timeClient->update();
        
        // Sync time from NTP to local system time
        if (timeClient->isTimeSet()) {
            if (!timeSynced) {
                timeSynced = true;
                setEpochTime(timeClient->getEpochTime());
                processBootEvent();
            } else {
                setEpochTime(timeClient->getEpochTime()); // keep it synced
            }
        }
    }

    unsigned long currentMillis = millis();
    
    if (timeSynced) {
        if (currentMillis - lastAliveUpdate >= 60000 || lastAliveUpdate == 0) {
            lastAliveUpdate = currentMillis;
            updateLastAlive();
        }
        
        // Rotate logs once a day (check every 24h roughly)
        if (currentMillis - lastLogRotation >= 86400000 || lastLogRotation == 0) {
            lastLogRotation = currentMillis;
            rotateLogs();
        }
    }
    
    if (requestTelegramTest) {
        requestTelegramTest = false;
        if (WiFi.status() == WL_CONNECTED) {
            sendTelegramMessage("✅ PowerMon bot connected successfully!");
        }
    }
    
    ElegantOTA.loop();
    webserverLoop();
}
