#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/twai.h"
#include "twai_task.h"

static const char* TAG = "TWAI_TASK";


/*  Example Code from driver/twai.h  */

void twai_init()
{
    twai_general_config_t twai_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    twai_config.alerts_enabled = TWAI_ALERT_ALL | TWAI_ALERT_AND_LOG;
    
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    ESP_ERROR_CHECK(twai_driver_install(&twai_config, &t_config, &f_config));
    ESP_LOGI(TAG, "TWAI Driver installed");

    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "TWAI Driver started");

    
}

void twai_rx_task(void *pvParameters)
{
    twai_init();
    

    /*      TWAI RX Example Code    */
    while(1)
    {
        //Wait for message to be received
        twai_message_t message;
        if (twai_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK) {
            printf("Message received\n");
        } else {
            printf("Failed to receive message\n");
            return;
        }

        //Process received message
        if (message.extd) {
            printf("Message is in Extended Format\n");
        } else {
            printf("Message is in Standard Format\n");
        }
        printf("ID is %lu\n", message.identifier);
        if (!(message.rtr)) {
            for (int i = 0; i < message.data_length_code; i++) {
                printf("Data byte %d = %d\n", i, message.data[i]);
            }
        }

                //Reconfigure alerts to detect Error Passive and Bus-Off error states
        uint32_t alerts_to_enable = TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF;
        if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
            printf("Alerts reconfigured\n");
        } else {
            printf("Failed to reconfigure alerts");
        }

        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}