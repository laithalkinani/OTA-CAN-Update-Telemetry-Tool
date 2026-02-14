#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "twai_task.h"

static const char* TAG = "TWAI_TASK";

/*  Self Test Mode  */

void twai_init()
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NO_ACK);
    g_config.alerts_enabled = TWAI_ALERT_ALL | TWAI_ALERT_AND_LOG;
    
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "TWAI Driver installed");

    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI Driver started");
}

void twai_rx_task(void *pvParameters)
{
    twai_init();
    
    twai_message_t tx_msg = {
        .identifier = 0x555,
        .data_length_code = 8,
        .self = 1,  // Self reception request
    };
    
    twai_message_t rx_msg;
    uint32_t tx_count = 0;
    uint32_t rx_count = 0;

    while(1)
    {
        // Transmit message every 100ms
        tx_msg.data[0] = tx_count & 0xFF;
        tx_msg.data[1] = (tx_count >> 8) & 0xFF;
        
        if (twai_transmit(&tx_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            ESP_LOGI(TAG, "TX %lu: ID=0x%lX", tx_count, tx_msg.identifier);
            tx_count++;
        } else {
            ESP_LOGE(TAG, "TX failed");
        }
        
        // Try to receive message
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) == ESP_OK) {
            ESP_LOGI(TAG, "RX %lu: ID=0x%lX Data=%02X %02X", 
                     rx_count, rx_msg.identifier, rx_msg.data[0], rx_msg.data[1]);
            rx_count++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}