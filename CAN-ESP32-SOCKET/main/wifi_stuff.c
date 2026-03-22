/*
wifi_stuff.c
Purpose: wi-fi init in esp32
Author: Laith Al-Kinani
*/

#include "esp_err.h"
#include "wifi_stuff.h" //function prototypes macros etc.
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define MAX_WIFI_CONNECT_RETRIES ((uint16_t)500)

static const char* WIFI_TAG = "WIFI_INIT";

static const char* SSID     =   "Al-Kinani Family";
static const char* password =   "Faith2008";

static uint16_t s_retry_num = 0;


static EventGroupHandle_t s_wifi_event_group;



static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        gpio_set_level(WIFI_STATUS_LED, 0);
        if (s_retry_num < MAX_WIFI_CONNECT_RETRIES)
        {
            esp_wifi_connect();         //try again
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "Retrying to connect to the AP...");
           
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "FAILED to connect to the AP.");
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        gpio_set_level(WIFI_STATUS_LED, 1);
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(WIFI_TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void initWifiSta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    gpio_reset_pin(WIFI_STATUS_LED);
    gpio_set_direction(WIFI_STATUS_LED, GPIO_MODE_OUTPUT);

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiEventHandler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifiEventHandler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid,     SSID,     sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "Wi-Fi Station Init completed.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Connected to AP with SSID: %s PW: %s", SSID, password);
        
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_TAG, "FAILED to connect to Wi-Fi...!!!!");
    }
    else
    {
        ESP_LOGI(WIFI_TAG, "Unexpected event!");
    }
    
}











