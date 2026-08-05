#pragma once
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void initWebServer();
void webserverLoop();
