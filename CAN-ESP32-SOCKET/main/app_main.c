/*
 * app_main.c
 */

#include <stdio.h>
#include <inttypes.h>
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "wifi_stuff.h"
#include "can_2_mqtt.h"
#include "mqtt_stuff.h"


void app_main(void)
{

ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(nvs_flash_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());

/*initialize the wifi station*/
initWifiSta();

/*initialize MQTT client*/
static can_2_mqtt_task_params_t mqttParams;
mqttParams.mqtt_client = initMqtt();

/*start the can_2_mqtt task*/
xTaskCreate(can_2_mqtt_task, "can_2_mqtt_task", 4096, &mqttParams, 5, NULL);

}