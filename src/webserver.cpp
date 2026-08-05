#include "webserver.h"
#include "api.h"
#include "settings.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

AsyncWebServer server(80);

static void serveIndexAuthenticated(AsyncWebServerRequest *request) {
    if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) {
        request->requestAuthentication();
        return;
    }
    request->send(LittleFS, "/index.html", "text/html");
}

void initWebServer() {
    setupAPI(server);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setAuthentication("admin", currentSettings.adminPassword.c_str(), AsyncAuthType::AUTH_BASIC);

    server.on("/", HTTP_GET, serveIndexAuthenticated);
    server.on("/dashboard", HTTP_GET, serveIndexAuthenticated);
    server.on("/dashboard*", HTTP_GET, serveIndexAuthenticated);
    server.on("/history", HTTP_GET, serveIndexAuthenticated);
    server.on("/history*", HTTP_GET, serveIndexAuthenticated);
    server.on("/daily", HTTP_GET, serveIndexAuthenticated);
    server.on("/daily*", HTTP_GET, serveIndexAuthenticated);
    server.on("/weekly", HTTP_GET, serveIndexAuthenticated);
    server.on("/weekly*", HTTP_GET, serveIndexAuthenticated);
    server.on("/monthly", HTTP_GET, serveIndexAuthenticated);
    server.on("/monthly*", HTTP_GET, serveIndexAuthenticated);
    server.on("/statistics", HTTP_GET, serveIndexAuthenticated);
    server.on("/statistics*", HTTP_GET, serveIndexAuthenticated);
    server.on("/settings", HTTP_GET, serveIndexAuthenticated);
    server.on("/settings*", HTTP_GET, serveIndexAuthenticated);

    server.onNotFound([](AsyncWebServerRequest *request){
        if (request->method() == HTTP_OPTIONS || request->method() == HTTP_HEAD) {
            request->send(200);
            return;
        }

        String uri = request->url();
        if (uri.startsWith("/api/") || uri.startsWith("/update") || uri.indexOf('.') >= 0) {
            request->send(404, "text/plain", "Not found");
            return;
        }

        if(!request->authenticate("admin", currentSettings.adminPassword.c_str())) {
            request->requestAuthentication();
            return;
        }

        request->send(LittleFS, "/index.html", "text/html");
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

    server.begin();
}

void webserverLoop() {
    // ESPAsyncWebServer doesn't require a loop function
}
