#pragma once
#include <Arduino.h>

bool sendTelegramMessage(const String& message, String token = "", String chatId = "");
void sendTelegramDocument(const char* filePath);
