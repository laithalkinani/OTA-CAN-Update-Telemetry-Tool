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


void app_main(void)
{

ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(nvs_flash_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());

CAN_Init();

/*  Begin MCP2515 read/write task   */
xTaskCreate(mcp2515_task, "MCP2515", 4096, NULL, 5, NULL);


CAN_EnableInterrupts();


}