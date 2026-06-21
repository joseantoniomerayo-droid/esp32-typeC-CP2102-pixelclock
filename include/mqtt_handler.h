#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

void initMQTT();
void mqttLoop();
void mqttPublishStatus();
bool isMQTTConnected();

extern PubSubClient mqttClient;
