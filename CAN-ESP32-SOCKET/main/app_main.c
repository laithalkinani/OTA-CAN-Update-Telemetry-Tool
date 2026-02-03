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
#include "mcp2515_driver.h"
#include "wifi_stuff.h"


/*
Brief: core 0 runs the can stuff, core 1 runs the wifi stuff. 
We run some error checks on the network init first, then we run
the CAN RX logic and the TCP client logic concurrently.

UPDATE: want to try running all can-tcp stuff on core 0 and all tcp-can stuff on core 1 (bidirectional)
with hardware synchronization on the CAN bus itself
*/

void app_main(void)
{

ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(nvs_flash_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());

CAN_Init();


}