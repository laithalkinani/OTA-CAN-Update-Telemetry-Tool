#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "can_2_mqtt.h"

static const char* TAG = "TWAI_TASK";
static twai_node_handle_t node_hdl = NULL;
static rx_msg_buffer_t rx_buffer;       //rx_buffer is the "glue" buffer between ISR and task, contains header and payload
static QueueHandle_t can_2_mqtt_queue = NULL;

/*  Forward Declarations    */

static bool twai_rx_done_callback(twai_node_handle_t handle,
                                            const twai_rx_done_event_data_t *edata,
                                            void *user_ctx);

/*  Point hardware FIFO buffer to "glue" buffer */
twai_frame_t rx_frame = {
    .buffer = rx_buffer.canPayload,         //now rx_frame.buffer points to rx_buffer.canPayload
    .buffer_len = sizeof(rx_buffer.canPayload),     //now points to size of payload (8 bytes)
};

/*  
@brief: init the twai controller node
*/
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

    /*  Now, create the queue   */
    /*  Creates 32-deep queue of rx_msg_buffer_t structs    */
    can_2_mqtt_queue = xQueueCreate(32, sizeof(rx_msg_buffer_t));

    //TODO: error handle better
    if (can_2_mqtt_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create can_2_mqtt_queue");
        return;
    }

    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
    ESP_LOGI(TAG, "TWAI node created");
    
    /*  Assign the TWAI on_rx_done ISR to our callback   */
    twai_event_callbacks_t callbacks = {
        .on_rx_done = twai_rx_done_callback,
    };

    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &callbacks, NULL));
    ESP_LOGI(TAG, "RX callback registered");
    
    /*  Enable the node -- always do this last!! */
    ESP_ERROR_CHECK(twai_node_enable(node_hdl));
    ESP_LOGI(TAG, "TWAI node enabled");

    uint64_t sizeOfCanFrame = sizeof(rx_buffer);
    ESP_LOGI(TAG, "Size of CAN Frame is: %llu", sizeOfCanFrame);


}


/*
@brief: TWAI RX callback placed in IRAM
@ret: bool that new msg has been queued
*/
static bool IRAM_ATTR twai_rx_done_callback(twai_node_handle_t handle,
                                            const twai_rx_done_event_data_t *edata,
                                            void *user_ctx)
{
    
    /*  "Interrupt flag" for callback   */
    BaseType_t higher_prio_woken = pdFALSE;
    
    /*  Read from hardware FIFO into rx_frame, which points to rx_buffer    */
    if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK)        //Note: we need to read from callback, according to API documentation
    {      
        /*  Copy header */
        rx_buffer.header = rx_frame.header;                             //Note: no len field needed because len is encoded in the DLC field of the CAN header
        
        /*  Timestamp   */
        rx_buffer.header.timestamp = (uint64_t)esp_timer_get_time();

        /*  Queue the message into the can_2_mqtt_queue */
        xQueueSendFromISR(can_2_mqtt_queue, &rx_buffer, &higher_prio_woken);
    }
    
    return (higher_prio_woken == pdTRUE);
}


static void flush_can2mqttbuffer(rx_msg_buffer_t* buffer)
{
    /*  TODO: actually send the full buffer to the MQTT packet here */
    ESP_LOGI(TAG, "Buffer full, flushing %d frames at %llu us", CAN_2_MQTT_BUFFER_SIZE, esp_timer_get_time());
    
    /*  Reset the buffer using memset, safe for a static buffer */
    memset(buffer, 0, CAN_2_MQTT_BUFFER_SIZE * sizeof(rx_msg_buffer_t));
}


void twai_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "TWAI RX Task started");
    
    twai_init();

    static rx_msg_buffer_t mqttBuffer[CAN_2_MQTT_BUFFER_SIZE] = {0};

    static uint8_t currentMqttBufferIndex = 0;

    
       while(1)
    {
        /*  Block until a frame arrives from ISR   */
        if (xQueueReceive(can_2_mqtt_queue, &mqttBuffer[currentMqttBufferIndex], portMAX_DELAY) == pdTRUE)
        {
            /*  Increment buffer index to next frame   */
            currentMqttBufferIndex++;

            /*  If index greater than max buffer size, flush buffer to MQTT and reset index */
            if (currentMqttBufferIndex >= CAN_2_MQTT_BUFFER_SIZE)
            {
                ESP_LOGI(TAG, "CAN_2_MQTT depth at flush: %lu", uxQueueMessagesWaiting(can_2_mqtt_queue));
                flush_can2mqttbuffer(mqttBuffer);
                currentMqttBufferIndex = 0;
            }
        }
    }

}