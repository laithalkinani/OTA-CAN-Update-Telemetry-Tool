/*
wifi_stuff.c
Purpose: TCP client in esp32
Author: Laith Al-Kinani
*/

#include "esp_err.h"
#include "wifi_stuff.h" //function prototypes macros etc.
#include "can_stuff.h"
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

esp_err_t createClient(void)
{
    //using ipv4 btw

    //init the socket address/port/stream 

    esp_err_t err = ESP_FAIL;
    char *payload = ascii_str; //this should be the CAN package
    //you may have to cast the can package to const char* later
    //just be careful because this is shared resource with core 0
    //so this will be a binary semaphore later
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP); //internet address
    dest_addr.sin_family = AF_INET; //using ipv4
    dest_addr.sin_port = htons(PORT); //which port we are binding our socket to

    //create the TCP socket

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ESP_LOGE(WIFI_TAG, "Unable to create socket: %d", errno);
        return err;
    }

    ESP_LOGI(WIFI_TAG, "Socket created, connected to %s:%d" HOST_IP, PORT);

    int ret = connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)); //bind socket to port
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
    //have something here to handle a successful send - i.e closing the socket still
}

void tcp_client(void *arg)
{
    createClient();
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



