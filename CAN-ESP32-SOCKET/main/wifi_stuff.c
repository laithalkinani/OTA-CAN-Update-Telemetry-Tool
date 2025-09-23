/*
wifi_stuff.c
Purpose: TCP client in esp32
Author: Laith Al-Kinani
*/

#include "esp_err.h"
#include "wifi_stuff.h" //function prototypes macros etc.
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

esp_err_t createClient(void)
{
    //init the socket address/port/stream 

    esp_err_t err = ESP_FAIL;
    char *payload = "Hello";
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    //create the TCP socket

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ESP_LOGE(WIFI_TAG, "Unable to create socket: %d", errno);
        return err;
    }

    ESP_LOGI(WIFI_TAG, "Socket created, connected to %s:%d" HOST_IP, PORT);

    int ret = connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (ret != 0)
    {
        ESP_LOGE(WIFI_TAG, "Unable to connect to socket: errno %d", errno);
        close(sock);
        return err;
    }
    ESP_LOGI(WIFI_TAG, "Successfully socketed!");

    //now send the data over TCP

    int meow = send(sock, payload, strlen(payload), 0);
    if (meow < 0)
    {
        ESP_LOGE(WIFI_TAG, "Error occured during sending: errno %d", errno);
        goto exit;
    }

    exit:
        shutdown(sock,0);
        close(sock);
        return err;
}

void tcp_client(void *arg)
{
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



