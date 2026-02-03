
/**
CORE0.C
Purpose: source code for Core 0 functionality - receive messages from CAN / send over TCP network
Created: Jan 13 2026
Author: Laith Al-Kinani

*/


 /**
  * CAN BUS NOTES
  * SPI Speed: 2 MHz
  * CAN Speed: 125 KBS
  * CAN Clock: 8 MHz
  */

 #include "core0.h"
 #include "core1.h"
 #include "ringbuffer.h"
 #include "can_stuff.h"
 #include <stdio.h>
 #include <stdbool.h>
 #include "can_stuff.h"
 #include "mcp2515.h"
 #include "driver/spi_master.h"
 #include "driver/gpio.h"
 #include "esp_err.h"
 #include "esp_log.h"
 #include <sys/socket.h>
 #include "esp_netif.h"
 #include "esp_wifi.h"
 #include <errno.h>
 #include <string.h>
 //TODO: include MQTT library later


 extern canBusContext* canBusMutex;     //use this to check if locked by core 1 (defined in core1.c)

 static currentCore0State core0StateStruct = {0};   //initialize core 0 states
 static const char *TAG = "CORE0_DEBUG";
 currentCore0State* core0State = &core0StateStruct; //pointer to struct
 


 void runCore0StateMachine(void)
 {
     /* Init Functions called once before SM executes    */
     
     core0State->currentState = CORE0_STATE0;        //initialize state to 0
     CAN_Init();
     ringbuffer_t rxBufferStruct = {0};
     ringbuffer_t* canEntryRxBuffer = &rxBufferStruct;
     ringbuffer_t  networkTxStruct = {0};
     ringbuffer_t* networkTxBuffer = &networkTxStruct;
     ringbuffer_init(canEntryRxBuffer);
     ringbuffer_init(networkTxBuffer);

    while(1)
    {

        switch (core0State->currentState)
        {
            case(CORE0_STATE0):

             if (canBusMutex->isBusLocked)      //CAN BUS LOCKED
             {
                core0State->nextState = CORE0_STATE0;       //stay in state0
             }
             else                                           //move to state1
             {
            
             core0State->nextState = CORE0_STATE1;
             }

            break;
            case(CORE0_STATE1):

                if (gpio_get_level(INT_PIN) == 0)           //INT Line pulled low, there is a message
                {
                ESP_LOGI(TAG, "INT PULLED LOW SUCCESSFULLY.");
                    
                    /*  check if canEntryRxBuffer is empty before queueing it   */

                    if (!isRingBufferFull(canEntryRxBuffer))        //ring buffer is empty
                    {
                        ringbuffer_queue(canEntryRxBuffer);          //stores CanEntry_t at HEAD

                        //TODO: do we go back to core0_state0 to check if can bus is locked?
                        //core0State->nextState = CORE0_STATE0;
                    }

                    else
                    {
                        /*  copy all contents to network_tx_buffer  */

                        ringbuffer_copy(canEntryRxBuffer, networkTxBuffer);

                        /*  reset rx_buffer */

                        ringbuffer_init(canEntryRxBuffer);

                        /*  go to next state    */
                        core0State->nextState = CORE0_STATE2;
                    }
                }

            break;
            case(CORE0_STATE2):

                core0State->nextState = CORE0_STATE0;

            break;
            default:

                core0State->nextState = CORE0_STATE0;

            break;
        }

    core0State->currentState = core0State->nextState;
    vTaskDelay(pdMS_TO_TICKS(10));                        //delay for polling
  }

}




 
 