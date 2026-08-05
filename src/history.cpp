#include "storage.h"
#include "utils.h"
#include "telegram.h"
#include "history.h"
#include <vector>

void updateLastAlive() {
    time_t now = getEpochTime();
    if (now > 1000000000) { 
        String ts = String(now);
        writeFile("/last_alive.txt", ts.c_str());
    }
}

void logEvent(EventType type, uint32_t duration, const String& description, time_t timestamp) {
    time_t ts = timestamp == 0 ? getEpochTime() : timestamp;
    if (ts < 1000000000) return; // Don't log if time is not synced
    
    JsonDocument doc;
    doc["timestamp"] = ts;
    doc["type"] = static_cast<int>(type);
    doc["duration"] = duration;
    doc["description"] = description;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    appendFile("/events.jsonl", (jsonStr + "\n").c_str());
}

void processBootEvent() {
    time_t now = getEpochTime();
    if (now < 1000000000) return; // Wait for valid time

    
    String lastAliveStr = readFile("/last_alive.txt");
    time_t lastAlive = lastAliveStr.toInt();
    
    if (isPowerLossReset()) {
        if (lastAlive > 0 && now > lastAlive) {
            uint32_t outageDuration = now - lastAlive;
            
            // Limit anomalous outage duration (e.g., first boot)
            if (outageDuration > 365 * 24 * 3600) outageDuration = 0;
            
            if (outageDuration > 60) {
                logEvent(EVENT_LOST, 0, "Electricity Lost", lastAlive);
                logEvent(EVENT_AVAILABLE, outageDuration, "Electricity Restored", now);
                
                // Count total outages and find longest from event log
                uint32_t totalCount = 0;
                uint32_t totalDuration = 0;
                uint32_t longest = 0;
                File file = LittleFS.open("/events.jsonl", "r");
                if (file) {
                    while (file.available()) {
                        String line = file.readStringUntil('\n');
                        if (line.length() < 2) continue;
                        JsonDocument doc;
                        if (!deserializeJson(doc, line)) {
                            int type = doc["type"];
                            if (type == EVENT_AVAILABLE) {
                                uint32_t dur = doc["duration"];
                                if (dur > 0) {
                                    totalCount++;
                                    totalDuration += dur;
                                    if (dur > longest) longest = dur;
                                }
                            }
                        }
                    }
                    file.close();
                }
                
                String msg = "⚡ <b>Power Restored!</b>\n\n";
                msg += "🔴 <b>Lost at:</b> " + formatDateTime(lastAlive) + "\n";
                msg += "🟢 <b>Back at:</b> " + formatDateTime(now) + "\n";
                msg += "⏱ <b>Outage:</b> " + formatDuration(outageDuration) + "\n";
                msg += "\n📊 <b>All-Time Stats:</b>\n";
                msg += "• Total outages: " + String(totalCount) + "\n";
                msg += "• Total downtime: " + formatDuration(totalDuration) + "\n";
                msg += "• Longest outage: " + formatDuration(longest) + "\n";
                if (totalCount > 0) {
                    msg += "• Average outage: " + formatDuration(totalDuration / totalCount);
                }
                
                sendTelegramMessage(msg);
            } else {
                logEvent(EVENT_AVAILABLE, 0, "Power Restored", now);
                sendTelegramMessage("🟢 Power Restored! (Brief drop &lt; 1 min)\n⏱ " + formatDateTime(now));
            }
        } else {
            logEvent(EVENT_AVAILABLE, 0, "First Boot / Power Restored", now);
            sendTelegramMessage("🟢 System Boot / Power Restored\n⏱ " + formatDateTime(now));
        }
    } else {
        logEvent(EVENT_RESTART, 0, "Device Restarted", now);
        sendTelegramMessage("🔵 Device Restarted (Manual/OTA)\n⏱ " + formatDateTime(now));
    }
    
    writeFile("/boot_time.txt", String(now).c_str());
}

void clearHistory() {
    deleteFile("/events.jsonl");
    logEvent(EVENT_SYSTEM, 0, "History Cleared");
}

int getTotalEvents() {
    File file = LittleFS.open("/events.jsonl", "r");
    if (!file) return 0;
    
    int count = 0;
    while(file.available()) {
        if (file.read() == '\n') count++;
    }
    file.close();
    return count;
}

String getEventsJson(int page, int limit) {
    File file = LittleFS.open("/events.jsonl", "r");
    if (!file) return "[]";
    
    int totalLines = 0;
    while(file.available()) {
        if (file.read() == '\n') totalLines++;
    }
    
    int startLine = totalLines - (page - 1) * limit;
    int endLine = startLine - limit; 
    if (endLine < 0) endLine = 0;
    
    file.seek(0, SeekSet);
    
    int currentLine = 0;
    std::vector<String> pageLines;
    while(file.available()) {
        String line = file.readStringUntil('\n');
        currentLine++;
        if (currentLine > endLine && currentLine <= startLine) {
            pageLines.push_back(line);
        }
    }
    file.close();
    
    String out = "[";
    bool first = true;
    for (int i = pageLines.size() - 1; i >= 0; i--) {
        if (pageLines[i].length() < 2) continue;
        if (!first) out += ",";
        out += pageLines[i];
        first = false;
    }
    out += "]";
    return out;
}

void rotateLogs() {
    time_t now = getEpochTime();
    if (now < 1000000000) return;
    
    FSInfo fs_info;
    LittleFS.info(fs_info);
    if (fs_info.usedBytes > fs_info.totalBytes * 0.85) {
        sendTelegramMessage("⚠️ Storage is over 85% full. Uploading backup and clearing old logs...");
        sendTelegramDocument("/events.jsonl");
        // Force retention to 7 days instead of 90 to clear space immediately
        time_t retentionLimit = now - (7 * 24 * 3600); 
        rotateLogsWithLimit(retentionLimit);
        return;
    }
    
    time_t retentionLimit = now - (90 * 24 * 3600); 
    rotateLogsWithLimit(retentionLimit);
}

void rotateLogsWithLimit(time_t retentionLimit) {
    File file = LittleFS.open("/events.jsonl", "r");
    if (!file) return;
    
    File temp = LittleFS.open("/events.tmp", "w");
    if (!temp) {
        file.close();
        return;
    }
    
    bool changed = false;
    while(file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() < 2) continue;
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, line);
        if (!err) {
            time_t ts = doc["timestamp"];
            if (ts >= retentionLimit) {
                temp.print(line + "\n");
            } else {
                changed = true;
            }
        }
    }
    
    file.close();
    temp.close();
    
    if (changed) {
        LittleFS.remove("/events.jsonl");
        LittleFS.rename("/events.tmp", "/events.jsonl");
    } else {
        LittleFS.remove("/events.tmp");
    }
}
