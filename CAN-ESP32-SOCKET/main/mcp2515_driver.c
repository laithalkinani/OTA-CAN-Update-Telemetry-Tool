/**
 * mcp2515_driver.c
 * Purpose: implement SPI and CAN functions, uses mcp2515.h library
 * Author: Laith Al-Kinani
 **/

 /**
  * SPI Speed: 2 MHz
  * CAN Speed: 500 KBS
  * CAN Clock: 8 MHz
  */

 #include <stdio.h>
 #include <stdbool.h>
 #include "mcp2515_driver.h"
 #include "mcp2515.h"
 #include "driver/spi_master.h"
 #include "driver/gpio.h"
 #include "esp_err.h"
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/queue.h"
 #include "freertos/semphr.h"

 /*     GLOBALS      */

 QueueHandle_t             rx_queue;  /*  Queue Handlers  */
 static TaskHandle_t      mcp2515_rx_wakeup_notif = NULL;

 /*     SPI HANDLERS    */

esp_err_t               ESP_ERR;
spi_device_handle_t     spi_handle;
static const char *TAG = "MCP2515_DRIVER";


/*  SPI INIT    */

bool SPI_Init(void)
{
    printf("Hello from SPI_Init!\n\r");
    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .miso_io_num = MISO_PIN,
        .mosi_io_num = MOSI_PIN,
        .sclk_io_num = SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 2000000,
        .spics_io_num = CS_PIN,
        .queue_size = 3
    };

    ret = spi_bus_initialize(ESP_HOST, &bus_cfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
    return false;
    }
    ESP_LOGI(TAG, "spi_bus_initialize OK");

    ret = spi_bus_add_device(ESP_HOST, &dev_cfg, &MCP2515_Object->spi);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
    return false;
    }
    ESP_LOGI(TAG, "spi_bus_add_device OK");
    ESP_LOGI(TAG, "SPI HANDLE: %p", MCP2515_Object->spi);


    return true;

}

/*     ISR - MCP2515 \INT PIN INTERRUPT HANDLER    */



 /*
 @brief: all this ISR does is wake up the MCP2515 task when /INT is pulled low
 */
 static void IRAM_ATTR mcp2515_ISR(void *arg) {
    gpio_set_level(LED, 1);         //latch LED on when interrupt is hit
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  //sets task woken to FALSE
    vTaskNotifyGiveFromISR(mcp2515_rx_wakeup_notif, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*      MCP2515 CAN CHIP INIT       */

/*
@brief: initializes the MCP2515, SPI bus, and /INT GPIO interrupt
@params: none
@ret: none
*/
 void CAN_Init(void)
 {

    ESP_LOGI(TAG, "Initializing MCP2515");

       //initialize the LED onboard the ESP32 devkit (pin 2) for debugging
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    //blink LED 3 times to confirm board is alive
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    MCP2515_init();     //creates MCP2515 object
    SPI_Init();

        uint8_t tx[3] = {0x03, 0x0E, 0x00}; // READ CANSTAT
        uint8_t rx[3] = {0};

        spi_transaction_t t = {
        .length = 8 * 3,
        .tx_buffer = tx,
        .rx_buffer = rx
        };

        gpio_set_level(CS_PIN, 0);
        spi_device_transmit(MCP2515_Object->spi, &t);
        gpio_set_level(CS_PIN, 1);

        ESP_LOGI("SPI", "RAW RX: %02X %02X %02X", rx[0], rx[1], rx[2]);

    MCP2515_reset();    //clears any buffers
    MCP2515_setBitrate(CAN_500KBPS, MCP_8MHZ);      //500 KBS bit rate
    MCP2515_setNormalMode();
    
    /*Diagnostics*/

    ESP_LOGI(TAG, "Reading CANCTRL: 0x%02X", MCP2515_readRegister(MCP_CANCTRL)); 
    //CANCTRL = 0x07 means normal operation mode enabled, sysclk/8, CLKOUT pin enabled
    ESP_LOGI(TAG, "Reading CANSTAT: 0x%02X", MCP2515_readRegister(MCP_CANSTAT));
    //CANCTRL = 0x0C means RXB0 interrupt which is good. 0x02 means error interrupt.


    //Mask/Filtering: can make this another function later
    MCP2515_setFilterMask(MASK0, false, 0x000);  // accept everything
    MCP2515_setFilterMask(MASK1, false, 0x000); //for now just for testing

    MCP2515_setFilter(0, false, 0x000);  // Accept all
    MCP2515_setFilter(1, false, 0x000);
    MCP2515_setFilter(2, false, 0x000);
    MCP2515_setFilter(3, false, 0x000);
    MCP2515_setFilter(4, false, 0x000);
    MCP2515_setFilter(5, false, 0x000);

    /*  Create the RX QUEUE (buffer can data into it)*/
    //TODO: make 32 a define macro later
    rx_queue = xQueueCreate(32, sizeof(CanEntry_t));    //queue 32 CanEntry_t structs which contains CAN frame + timestamp

  

    /* Set gpio /INT pin as input (PULLED UP) (you sure it's pulled up dude?) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,           //TODO: investigate if this really is pulled up or not
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE, //falling edge interrupt. TODO: explore GPIO_INTR_LOW_LEVEL
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);        //creates ISR service with flag 0 (default)
    
    gpio_isr_handler_add(INT_PIN, mcp2515_ISR, NULL);   //attaches ISR handler to INT_PIN

    //at this point the ISR has been enabled, mcp2515_ISR gives semaphore to wake up task every time /INT pulled low
    
    ESP_LOGI(TAG, "MCP2515 driver successfully initialized...!  ");


 }


/*
@brief: sleeps until /INT wakes it up; then proceeds to read CAN msg over SPI.
        Owner of the MCP2515 driver.
@params: *pvParameters; config pointer
@ret: none
*/
void mcp2515_task(void *pvParameters) 
{
    ESP_LOGI(TAG, "Starting MCP2515 polling task...");
    
    struct can_frame frame;
    CAN_FRAME framePtr = &frame;
    char ascii_str[9]; // Buffer for ASCII data (8 bytes + null terminator)
    
    // Give the MCP2515 a moment to stabilize after init
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1) 
    {
        // Poll for received messages using the RX status check
        uint8_t rx_status = MCP2515_checkReceive();
        
        if (rx_status != 0) {
            // Turn on LED to indicate message reception
            gpio_set_level(LED, 1);
            
            ESP_LOGI(TAG, "Message detected! RX_STATUS: 0x%02X", rx_status);
            
            // Read the message
            ERROR_t err = MCP2515_readMessageAfterStatCheck(framePtr);
            
            if (err == ERROR_OK) {
                // Log frame information
                ESP_LOGI(TAG, "FRAME ID: 0x%08X", (unsigned int)framePtr->can_id);
                ESP_LOGI(TAG, "DLC: %d", framePtr->can_dlc);
                
                // Convert data to ASCII string if printable
                if (framePtr->can_dlc > 0 && framePtr->can_dlc <= 8) {
                    memcpy(ascii_str, framePtr->data, framePtr->can_dlc);
                    ascii_str[framePtr->can_dlc] = '\0';
                    ESP_LOGI(TAG, "DATA (ASCII): %s", ascii_str);
                    
                    // Also log hex values
                    ESP_LOG_BUFFER_HEX(TAG, framePtr->data, framePtr->can_dlc);
                }
                
                // Queue the message with timestamp
                CanEntry_t entry = {
                    .canFrame = framePtr,  // Copy the whole struct, not just pointer
                    .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS
                };
                
                if (xQueueSend(rx_queue, &entry, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "RX queue full, message dropped");
                }
                
                // Verify INT pin state
                if (gpio_get_level(INT_PIN) == 0) {
                    ESP_LOGI(TAG, "INT pin LOW (correct)");
                } else {
                    ESP_LOGW(TAG, "INT pin HIGH (unexpected)");
                }
                
                // Turn off LED after processing
                gpio_set_level(LED, 0);
                
            } else {
                ESP_LOGE(TAG, "Read error: %d", err);
                
                // Log diagnostic information
                uint8_t canintf = MCP2515_readRegister(MCP_CANINTF);
                uint8_t eflg = MCP2515_readRegister(MCP_EFLG);
                ESP_LOGE(TAG, "CANINTF: 0x%02X, EFLG: 0x%02X", canintf, eflg);
            }
            
            // Check for errors after reading
            uint8_t error_flags = MCP2515_getErrorFlags();
            if (error_flags != 0) {
                ESP_LOGW(TAG, "ERROR FLAGS: 0x%02X", error_flags);
            }
        }
        
        // Poll every 10ms (100 Hz polling rate)
        // Adjust this based on your expected message rate
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

 

 
//   void CAN_Polling(void *arg)
//   {
//     // //The naming is soo confusing here ngl 
//     //Basically just creating a pointer to reference a struct.
//     struct can_frame frame_struct;   //actual frame
//     CAN_FRAME frame = &frame_struct; //pointer to pass the frame

//     CAN_Init();
//     MCP2515_readMessageAfterStatCheck(frame);
  
//     ESP_LOGI(TAG, "RX STATUS: %d",MCP2515_checkReceive());
//     uint8_t intf = MCP2515_readRegister(MCP_CANINTF);
//     ESP_LOGI(TAG, "CANINTF after clear: 0x%02X", intf);
//     //CANINTF = 0x02 means int flag on RXB1
//     ESP_LOGI(TAG, "CHECK ERRORS: 0x%02X", MCP2515_checkError());
//     ESP_LOGI(TAG, "ANY ERRORS: 0x%02X", MCP2515_getErrorFlags());
    

//     //bunch of diagnostics
//     ESP_LOGI(TAG, "GET STATUS: 0x%02X", MCP2515_getStatus());
//     ESP_LOGI(TAG, "CHECK MSG RECEIVE: %d", (uint32_t)MCP2515_readMessageAfterStatCheck(frame));
//     ESP_LOGI(TAG, "FRAME ID: 0x%08X", (unsigned int long)frame->can_id);
//     ESP_LOGI(TAG, "DLC: %d", frame->can_dlc);

    
//     memcpy(ascii_str, frame->data, frame->can_dlc); //copy frame->data into ascii_str (in general memcpy is not good, just for testing here)
//     ascii_str[frame->can_dlc] = '\0'; //add null terminator for C string, hello \0
//     ESP_LOGI(TAG, "DATA (ASCII): %s", ascii_str); //should say hello

//     if (gpio_get_level(INT_PIN) != 0)
//     {
//         ESP_LOGI(TAG, "INT NOT PULLED LOW. RX FAIL.");
//     }
//     else
//     {
//         ESP_LOGI(TAG, "INT PULLED LOW SUCCESSFULLY.");
//     }




//             //Next steps: figure out how to do this repeatedly, aka clear
//             //the RXbuffer to make room for the next msg?
//             //figure out how this is done

//     while(1) //TODO: be careful having a while(1) loop here... 
//     //Because what if you have a while(1) loop running for the wifi stuff?
//     //Now you have to use both cores.
//     {
//         vTaskDelay(pdMS_TO_TICKS(10)); //delay for polling.

//     }

 





