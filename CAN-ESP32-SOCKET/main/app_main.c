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
#include "can_stuff.h"
#include "wifi_stuff.h"


/*
Brief: core 0 runs the can stuff, core 1 runs the wifi stuff. 
We run some error checks on the network init first, then we run
the CAN RX logic and the TCP client logic concurrently.
*/

void app_main(void)
{

ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(nvs_flash_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());

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

//pin the wifi stuff to core 1

xTaskCreatePinnedToCore
(
    tcp_client,
    "Setting up TCP Client...",
    4096,
    NULL,
    1,
    NULL,
    1 //core 1

);


}