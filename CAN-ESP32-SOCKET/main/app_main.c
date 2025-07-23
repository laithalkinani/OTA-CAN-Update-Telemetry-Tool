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
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "mcp2515.h"
#include "can_stuff.h"
#include "wifi_stuff.h"


#define GREEN_LED 13
#define BLUE_LED 26



 void configureGpios(void)
 {
    gpio_reset_pin(GREEN_LED);
    gpio_reset_pin(BLUE_LED);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
 }

 void blinkGreenLed (void* arg)
 {
    while (1) 
    {
    gpio_set_level(GREEN_LED, 1);
    vTaskDelay( 1000 / portTICK_PERIOD_MS); 
    gpio_set_level(GREEN_LED, 0);
    vTaskDelay( 1000 / portTICK_PERIOD_MS);
    }
 }

 void blinkBlueLed (void* arg) 
 {

    while (1)
    {
    gpio_set_level(BLUE_LED, 1);
    vTaskDelay( 500 / portTICK_PERIOD_MS); 
    gpio_set_level(BLUE_LED, 0);
    vTaskDelay( 500 / portTICK_PERIOD_MS);
    }

 }

void app_main(void)
{
    configureGpios();

//create task 1 for green LED and pin it to core 0

xTaskCreatePinnedToCore 
(
    blinkGreenLed,
    "Blink Green LED",
    2048,
    NULL,
    1,
    NULL,
    0 //core 0
);

//create task 2 for blue led and pin it to core 1

xTaskCreatePinnedToCore
(
    blinkBlueLed,
    "Blink Blue LED",
    2048,
    NULL,
    1,
    NULL,
    1 //core 1
);

}