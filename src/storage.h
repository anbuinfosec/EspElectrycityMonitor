#pragma once
#include <Arduino.h>
#include <LittleFS.h>

bool initStorage();
String readFile(const char* path);
bool writeFile(const char* path, const char* message);
bool appendFile(const char* path, const char* message);
bool deleteFile(const char* path);
bool fileExists(const char* path);
size_t getFileSize(const char* path);
