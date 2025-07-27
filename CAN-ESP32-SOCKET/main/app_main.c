/*
 * Project started as hello_world example from ESP IDF 
 * Outline: blink two LEDs concurrently, out of phase, using two tasks 
 * running independently of each other. 
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "can_stuff.h"
#include "wifi_stuff.h"



void app_main(void)
{


xTaskCreatePinnedToCore 
(
    CAN_Polling,
    "Polling for CAN...",
    2048,
    NULL,
    1,
    NULL,
    0 //core 0
);


}