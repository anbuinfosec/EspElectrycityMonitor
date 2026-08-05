#include "reports.h"
#include "history.h"
#include "storage.h"
#include "utils.h"
#include <ArduinoJson.h>
#include <map>

String getDayString(time_t t) {
    struct tm * ti = localtime(&t);
    if (!ti) {
        return String("");
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
            ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);
    return String(buf);
}

String getWeekString(time_t t) {
    struct tm * ti = localtime(&t);
    if (!ti) {
        return String("");
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-W%02d",
            ti->tm_year + 1900, ti->tm_yday / 7 + 1);
    return String(buf);
}

String getMonthString(time_t t) {
    struct tm * ti = localtime(&t);
    if (!ti) {
        return String("");
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d",
            ti->tm_year + 1900, ti->tm_mon + 1);
    return String(buf);
}

String getStatisticsJson() {
    File file = LittleFS.open("/events.jsonl", "r");
    uint32_t totalOutage = 0;
    uint32_t totalOutagesCount = 0;
    uint32_t longestOutage = 0;
    uint32_t shortestOutage = 0xFFFFFFFF;
    time_t lastOutage = 0;
    
    if (file) {
        while(file.available()) {
            String line = file.readStringUntil('\n');
            if(line.length() < 2) continue;
            JsonDocument doc;
            if(!deserializeJson(doc, line)) {
                int type = doc["type"];
                if (type == EVENT_AVAILABLE) { 
                    uint32_t duration = doc["duration"];
                    if (duration > 0) { 
                        totalOutage += duration;
                        totalOutagesCount++;
                        if (duration > longestOutage) longestOutage = duration;
                        if (duration < shortestOutage) shortestOutage = duration;
                    }
                } else if (type == EVENT_LOST) {
                    lastOutage = doc["timestamp"];
                }
            }
        }
        file.close();
    }
    
    if (shortestOutage == 0xFFFFFFFF) shortestOutage = 0;
    uint32_t avgOutage = totalOutagesCount > 0 ? totalOutage / totalOutagesCount : 0;
    
    time_t now = getEpochTime();
    String bootTimeStr = readFile("/boot_time.txt");
    time_t bootTime = bootTimeStr.toInt();
    uint32_t sessionTime = now > bootTime ? now - bootTime : 0;
    
    JsonDocument out;
    out["totalOutage"] = totalOutage;
    out["totalOutagesCount"] = totalOutagesCount;
    out["longestOutage"] = longestOutage;
    out["shortestOutage"] = shortestOutage;
    out["averageOutage"] = avgOutage;
    out["lastOutage"] = lastOutage;
    out["currentSession"] = sessionTime;
    
    String jsonStr;
    serializeJson(out, jsonStr);
    return jsonStr;
}

String getDailyReportJson() {
    std::map<String, uint32_t> dailyOutage;
    std::map<String, uint32_t> dailyCount;
    
    File file = LittleFS.open("/events.jsonl", "r");
    if (file) {
        while(file.available()) {
            String line = file.readStringUntil('\n');
            if(line.length() < 2) continue;
            JsonDocument doc;
            if(!deserializeJson(doc, line)) {
                int type = doc["type"];
                if (type == EVENT_AVAILABLE) {
                    uint32_t duration = doc["duration"];
                    time_t ts = doc["timestamp"];
                    if (duration > 0) {
                        String key = getDayString(ts);
                        dailyOutage[key] += duration;
                        dailyCount[key]++;
                    }
                }
            }
        }
        file.close();
    }
    
    JsonDocument out;
    JsonArray arr = out.to<JsonArray>();
    
    for (auto const& pair : dailyOutage) {
        JsonObject obj = arr.add<JsonObject>();
        obj["date"] = pair.first;
        obj["outageDuration"] = pair.second;
        obj["outageCount"] = dailyCount[pair.first];
    }
    
    String jsonStr;
    serializeJson(out, jsonStr);
    return jsonStr;
}

String getWeeklyReportJson() {
    std::map<String, uint32_t> weeklyOutage;
    std::map<String, uint32_t> weeklyCount;
    
    File file = LittleFS.open("/events.jsonl", "r");
    if (file) {
        while(file.available()) {
            String line = file.readStringUntil('\n');
            if(line.length() < 2) continue;
            JsonDocument doc;
            if(!deserializeJson(doc, line)) {
                int type = doc["type"];
                if (type == EVENT_AVAILABLE) {
                    uint32_t duration = doc["duration"];
                    time_t ts = doc["timestamp"];
                    if (duration > 0) {
                        String key = getWeekString(ts);
                        weeklyOutage[key] += duration;
                        weeklyCount[key]++;
                    }
                }
            }
        }
        file.close();
    }
    
    JsonDocument out;
    JsonArray arr = out.to<JsonArray>();
    for (auto const& pair : weeklyOutage) {
        JsonObject obj = arr.add<JsonObject>();
        obj["week"] = pair.first;
        obj["outageDuration"] = pair.second;
        obj["outageCount"] = weeklyCount[pair.first];
    }
    String jsonStr;
    serializeJson(out, jsonStr);
    return jsonStr;
}

String getMonthlyReportJson() {
    std::map<String, uint32_t> monthlyOutage;
    std::map<String, uint32_t> monthlyCount;
    
    File file = LittleFS.open("/events.jsonl", "r");
    if (file) {
        while(file.available()) {
            String line = file.readStringUntil('\n');
            if(line.length() < 2) continue;
            JsonDocument doc;
            if(!deserializeJson(doc, line)) {
                int type = doc["type"];
                if (type == EVENT_AVAILABLE) {
                    uint32_t duration = doc["duration"];
                    time_t ts = doc["timestamp"];
                    if (duration > 0) {
                        String key = getMonthString(ts);
                        monthlyOutage[key] += duration;
                        monthlyCount[key]++;
                    }
                }
            }
        }
        file.close();
    }
    
    JsonDocument out;
    JsonArray arr = out.to<JsonArray>();
    for (auto const& pair : monthlyOutage) {
        JsonObject obj = arr.add<JsonObject>();
        obj["month"] = pair.first;
        obj["outageDuration"] = pair.second;
        obj["outageCount"] = monthlyCount[pair.first];
    }
    String jsonStr;
    serializeJson(out, jsonStr);
    return jsonStr;
}
