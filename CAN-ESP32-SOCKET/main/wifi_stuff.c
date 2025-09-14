/*
wifi_stuff.c
Purpose: TCP client in esp32
Author: Laith Al-Kinani
*/

#include "wifi_stuff.h" //function prototypes macros etc.
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

void tcp_client(void *arg)
{
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



