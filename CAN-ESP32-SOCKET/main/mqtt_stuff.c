

/*
mqtt_stuff.c - where mqtt setup lives
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "esp_log.h"
#include "mqtt_stuff.h"
#include "mqtt_client.h"

static const char* BROKER_ADDRESS = "mqtt://24.57.66.230:1883";

static const char* MQTT_TAG = "MQTT_STUFF";
static EventGroupHandle_t s_mqtt_event_group;

#define MQTT_CONNECTED_BIT      BIT0
#define MQTT_DISCONNECTED_BIT   BIT1
#define MQTT_ERROR_BIT          BIT2


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

#define MQTT_TEST 1 //0 for production, 1 for debugging/test

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            xEventGroupClearBits(s_mqtt_event_group, MQTT_DISCONNECTED_BIT);
            esp_mqtt_client_subscribe(event->client, "can/frames", 0);
            #if MQTT_TEST
            ESP_LOGI(MQTT_TAG, "MQTT connected.");
            #endif
            break;

        case MQTT_EVENT_DISCONNECTED:
            xEventGroupSetBits(s_mqtt_event_group, MQTT_DISCONNECTED_BIT);
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            #if MQTT_TEST
            ESP_LOGW(MQTT_TAG, "MQTT disconnected.");
            #endif
            break;

        case MQTT_EVENT_PUBLISHED:
            #if MQTT_TEST
            ESP_LOGI(MQTT_TAG, "MQTT published, msg_id=%d", event->msg_id);
            #endif
            break;

        case MQTT_EVENT_DATA:
            #if MQTT_TEST
            ESP_LOGI(MQTT_TAG, "MQTT data received.");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n",  event->data_len,  event->data);
            #endif
            break;

        case MQTT_EVENT_ERROR:
            xEventGroupSetBits(s_mqtt_event_group, MQTT_ERROR_BIT);
            #if MQTT_TEST
            ESP_LOGE(MQTT_TAG, "MQTT error: type=%d", event->error_handle->error_type);
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_error_if_nonzero("esp-tls",      event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("tls stack",    event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("socket errno", event->error_handle->esp_transport_sock_errno);
            }
            #endif
            break;

        default:
            break;
    }
}


esp_mqtt_client_handle_t initMqtt(void)
{
    s_mqtt_event_group = xEventGroupCreate();

    //esp config client

    esp_mqtt_client_config_t mqtt_cfg =
    {
        .broker.address.uri = BROKER_ADDRESS,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI(MQTT_TAG, "MQTT client started, waiting for connection...");

    EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group,
                                           MQTT_CONNECTED_BIT | MQTT_ERROR_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    
    if (bits & MQTT_CONNECTED_BIT)
    {
        ESP_LOGI(MQTT_TAG, "MQTT Client Connected to Broker...!!!");
    } 
    else if (bits & MQTT_ERROR_BIT)
    {
        ESP_LOGE(MQTT_TAG, "Failed to connect to MQTT broker.");
    }

    return client;                                     

}


void mqttWaitUntilConnected(void)
{
    xEventGroupWaitBits(s_mqtt_event_group,
                        MQTT_CONNECTED_BIT,
                        pdFALSE,       // don't clear the bit
                        pdTRUE,
                        portMAX_DELAY);
}




