#include "telegram.h"
#include "settings.h"
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <memory>

bool sendTelegramMessage(const String& message, String token, String chatId) {
    if (token == "") token = currentSettings.telegramBotToken;
    if (chatId == "") chatId = currentSettings.telegramChatId;
    
    if (token == "" || chatId == "") return false;
    
#ifdef ESP32
    std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
#else
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#endif
    client->setInsecure();
    
    HTTPClient https;
    String url = "https://api.telegram.org/bot" + token + "/sendMessage";
    
    bool success = false;
    if (https.begin(*client, url)) {
        https.addHeader("Content-Type", "application/json");
        
        JsonDocument doc;
        doc["chat_id"] = chatId;
        doc["text"] = message;
        doc["parse_mode"] = "HTML";
        
        String payload;
        serializeJson(doc, payload);
        
        int httpCode = https.POST(payload);
        if (httpCode == 200) {
            success = true;
        }
        https.end();
    }
    return success;
}

void sendTelegramDocument(const char* filePath) {
    if (!currentSettings.telegramEnabled || currentSettings.telegramBotToken == "" || currentSettings.telegramChatId == "") {
        return;
    }
    
    File file = LittleFS.open(filePath, "r");
    if (!file) return;
    size_t fileSize = file.size();
    
#ifdef ESP32
    std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
#else
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
#endif
    client->setInsecure();
    
    if (client->connect("api.telegram.org", 443)) {
        String boundary = "----ESP8266Boundary";
        String head = "--" + boundary + "\r\n"
                    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + 
                    currentSettings.telegramChatId + "\r\n" +
                    "--" + boundary + "\r\n"
                    "Content-Disposition: form-data; name=\"document\"; filename=\"events.jsonl\"\r\n"
                    "Content-Type: application/json\r\n\r\n";
        String tail = "\r\n--" + boundary + "--\r\n";
        
        uint32_t contentLength = head.length() + fileSize + tail.length();
        
        client->print("POST /bot" + currentSettings.telegramBotToken + "/sendDocument HTTP/1.1\r\n");
        client->print("Host: api.telegram.org\r\n");
        client->print("Content-Length: " + String(contentLength) + "\r\n");
        client->print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
        client->print("Connection: close\r\n\r\n");
        
        client->print(head);
        
        uint8_t buffer[512];
        while (file.available()) {
            size_t len = file.read(buffer, sizeof(buffer));
            client->write(buffer, len);
        }
        
        client->print(tail);
        file.close();
        
        while(client->connected()) {
            String line = client->readStringUntil('\n');
            if (line == "\r") {
                break;
            }
        }
    }
}
