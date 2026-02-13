

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_twai.h"
#include "driver/twai.h"
#include "esp_twai_onchip.h"
#include "twai_task.h"
 #include "freertos/task.h"

static const char* TAG = "TWAI_TASK";


// static TaskHandle_t rx_task_handle = NULL;


void twai_init()
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_RX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();


     if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) 
     {
        ESP_LOGI(TAG, "TWAI Driver Installed Successfully!");
     } 
        else 
        {
        ESP_LOGE(TAG, "TWAI Driver Failed to Install, Exiting init...");
        return;
        }

    //Start TWAI driver
    if (twai_start() == ESP_OK) 
    {
         ESP_LOGI(TAG, "TWAI Driver Started Successfully! ");
    } 
        else 
        {
         ESP_LOGE(TAG, "TWAI Driver Failed To Start... ");
        return;
        }
}


void twai_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "TWAI RX Task started");
    twai_init();
    
    twai_message_t message;
    uint32_t alerts;
    twai_status_info_t status;

    while(1)
    {
        // Check for alerts (non-blocking)
        if (twai_read_alerts(&alerts, pdMS_TO_TICKS(0)) == ESP_OK) 
        {
            if (alerts & TWAI_ALERT_BUS_ERROR) {
                ESP_LOGW(TAG, "Bus Error!");
            }
            if (alerts & TWAI_ALERT_ERR_PASS) {
                ESP_LOGW(TAG, "Error Passive state!");
            }
            if (alerts & TWAI_ALERT_BUS_OFF) {
                ESP_LOGE(TAG, "Bus-Off state! Starting recovery...");
                twai_initiate_recovery();
            }
            if (alerts & TWAI_ALERT_ABOVE_ERR_WARN) {
                ESP_LOGW(TAG, "Above error warning limit!");
            }
        }

        // Try to receive message
        esp_err_t ret = twai_receive(&message, pdMS_TO_TICKS(1000));
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "RX: ID=0x%lX DLC=%d", message.identifier, message.data_length_code);
            for (int i = 0; i < message.data_length_code; i++)
            {
                printf("Data[%d] = 0x%02X ", i, message.data[i]);
            }
            printf("\n");
        }
        else if (ret == ESP_ERR_TIMEOUT)
        {
            // Check status every timeout
            if (twai_get_status_info(&status) == ESP_OK) {
                ESP_LOGI(TAG, "Status - State:%d TEC:%lu REC:%lu RX_miss:%lu Bus_err:%lu", 
                         status.state, status.tx_error_counter, status.rx_error_counter,
                         status.rx_missed_count, status.bus_error_count);
            }
        }
        else
        {
            ESP_LOGE(TAG, "RX error: %s", esp_err_to_name(ret));
        }
    }
}



