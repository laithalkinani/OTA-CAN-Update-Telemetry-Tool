#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "twai_task.h"

static const char* TAG = "TWAI_TASK";
static TaskHandle_t rx_task_handle = NULL;
static twai_node_handle_t node_hdl = NULL;



static rx_msg_buffer_t rx_buffer;       //rx_buffer is the "glue" buffer between ISR and task, contains header and payload

/*  Point hardware FIFO buffer to glue buffer */
twai_frame_t rx_frame = {
    .buffer = rx_buffer.canPayload,         //now rx_frame.buffer points to rx_buffer.canPayload
    .buffer_len = sizeof(rx_buffer.canPayload),     //now points to size of payload (8 bytes)
};

/*
@brief: TWAI RX callback placed in IRAM
*/
static bool IRAM_ATTR twai_rx_done_callback(twai_node_handle_t handle,
                                            const twai_rx_done_event_data_t *edata,
                                            void *user_ctx)
{
    BaseType_t higher_prio_woken = pdFALSE;
    
    
    /*  Read from hardware FIFO into rx_frame, which points to rx_buffer    */
    if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK)        //Note: we need to read from callback, according to API documentation
    {      
        /*  Copy header */
        rx_buffer.header = rx_frame.header;                             //Note: no len field needed because len is encoded in the DLC field of the CAN header
    
        /*  Notify twai_rx_task that data is ready  */
        vTaskNotifyGiveFromISR(rx_task_handle, &higher_prio_woken);
    }
    
    return (higher_prio_woken == pdTRUE);
}

void twai_init()
{
    twai_onchip_node_config_t node_config = {
        .io_cfg.tx = TWAI_TX_PIN,
        .io_cfg.rx = TWAI_RX_PIN,
        .io_cfg.quanta_clk_out = -1,
        .io_cfg.bus_off_indicator = -1,
        .bit_timing.bitrate = TWAI_BITRATE,
        .tx_queue_depth = 5,
        .intr_priority = 1,
        .flags.enable_self_test = 0,
        .flags.enable_loopback = 0,
        .flags.enable_listen_only = 0,
    };

    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
    ESP_LOGI(TAG, "TWAI node created");
    
    twai_event_callbacks_t callbacks = {
        .on_rx_done = twai_rx_done_callback,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &callbacks, NULL));
    ESP_LOGI(TAG, "RX callback registered");
    
    ESP_ERROR_CHECK(twai_node_enable(node_hdl));
    ESP_LOGI(TAG, "TWAI node enabled");
}

void twai_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "TWAI RX Task started");
    
    rx_task_handle = xTaskGetCurrentTaskHandle();
    
    twai_init();
    
    while(1)
    {
        /*  Block until woken by ISR    */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /*  Process msg received in ISR     */
        //TODO: add timestamp here or in ISR?
        ESP_LOGI(TAG, "RX: ID=0x%lX [%d]", rx_buffer.header.id, rx_buffer.header.dlc);
        
        if (!rx_buffer.header.rtr) {
            printf("Data: ");
            for (int i = 0; i < rx_buffer.header.dlc; i++) {
                printf("%02X ", rx_buffer.canPayload[i]);
            }
            printf("\n");
        }
    }
}