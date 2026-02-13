

#ifndef CAN_TASK_H
#define CAN_TASK_H


#define TWAI_RX_PIN 22
#define TWAI_TX_PIN 21
#define TWAI_BITRATE    500000


void twai_init();
void twai_rx_task(void *pvParameters);





#endif //CAN_TASK_H