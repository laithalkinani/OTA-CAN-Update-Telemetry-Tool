

#ifndef CAN_2_MQTT_H
#define CAN_2_MQTT_H

#include "esp_twai.h"
#include "esp_twai_onchip.h"


#define TWAI_RX_PIN 22
#define TWAI_TX_PIN 21
#define TWAI_BITRATE    500000
#define CAN_2_MQTT_BUFFER_SIZE 32


/*  Buffer to pass msg from rx_callback to twai_rx_task     */
typedef struct {
    twai_frame_header_t header;
    uint8_t canPayload[8];      //size of payload for classic CAN
} rx_msg_buffer_t;


/*  Function Prototypes */
void twai_init();
void twai_rx_task(void *pvParameters);





#endif //CAN_2_MQTT_H