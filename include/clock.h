#pragma once
#include <ArduinoJson.h>

bool syncNTP();
void fetchWeather();
void  showConnecting(const char* msg);
void  setCalendarEvents(JsonArray arr);
void  drawClock();
