

/*
mqtt_stuff.c - where mqtt setup lives
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "esp_log.h"
#include "mqtt_stuff.h"
#include "mqtt_client.h"

static const char* TAG = "MQTT_STUFF";