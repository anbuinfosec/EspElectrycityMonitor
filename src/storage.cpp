#include "storage.h"

bool initStorage() {
    if (!LittleFS.begin()) {
        Serial.println("Error mounting LittleFS");
        return false;
    }
    return true;
}

String readFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        return "";
    }
    String fileContent = file.readString();
    file.close();
    return fileContent;
}

bool writeFile(const char* path, const char* message) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        return false;
    }
    size_t written = file.print(message);
    file.close();
    return written > 0;
}

bool appendFile(const char* path, const char* message) {
    File file = LittleFS.open(path, "a");
    if (!file) {
        return false;
    }
    size_t written = file.print(message);
    file.close();
    return written > 0;
}

bool deleteFile(const char* path) {
    return LittleFS.remove(path);
}

bool fileExists(const char* path) {
    return LittleFS.exists(path);
}

size_t getFileSize(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        return 0;
    }
    size_t size = file.size();
    file.close();
    return size;
}
