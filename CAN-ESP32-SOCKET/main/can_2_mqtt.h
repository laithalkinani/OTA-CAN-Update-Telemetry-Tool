

#ifndef CAN_2_MQTT_H
#define CAN_2_MQTT_H

#include "esp_twai.h"
#include "esp_twai_onchip.h"


#define TWAI_RX_PIN 22
#define TWAI_TX_PIN 21
#define TWAI_BITRATE    500000
#define CAN_2_MQTT_BUFFER_SIZE 32

/*  This is a custom struct, based on the twai_fra*/
typedef struct __attribute__((packed))      //forcing the header to be packed, to make parsing easier down the line
{
    uint32_t id;
    uint16_t dlc;
    uint8_t  flags;
    uint64_t timestamp;

} can_frame_header_packed_t;

/*  Buffer to pass msg from rx_callback to twai_rx_task     */
typedef struct __attribute__((packed))
{
    can_frame_header_packed_t header;
    uint8_t canPayload[8];
} rx_msg_buffer_t;


/*  Function Prototypes */
void twai_init();
void can_2_mqtt_task(void *pvParameters);





#endif //CAN_2_MQTT_H