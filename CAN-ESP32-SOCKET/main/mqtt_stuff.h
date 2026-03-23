#ifndef MQTT_STUFF_H
#define MQTT_STUFF_H

#include "mqtt_client.h"

const char* CAN_2_MQTT_TOPIC = "can/frames";

/* global context struct, can be shared between can2mqtt and mqtt2can tasks */
typedef struct {
    esp_mqtt_client_handle_t mqtt_client;
} can_2_mqtt_task_params_t;

esp_mqtt_client_handle_t initMqtt(void);
void mqttWaitUntilConnected(void);


#endif //MQTT_STUFF_H