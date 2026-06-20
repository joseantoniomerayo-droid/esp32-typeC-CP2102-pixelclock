#pragma once

void initWiFi();
bool syncNTP();
void fetchWeather();
void showConnecting(const char* msg);
void drawClock();
void debugSetWeather(int code, float temp);
