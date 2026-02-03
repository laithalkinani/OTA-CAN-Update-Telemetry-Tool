/**
 * mcp2515_driver.c
 * Purpose: implement SPI and CAN functions
 * Author: Laith Al-Kinani
 **/

 /**
  * SPI Speed: 2 MHz
  * CAN Speed: 125 KBS
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

 /******GLOBALS************** */

 QueueHandle_t rx_queue;  /*  Queue Handlers  */
 static SemaphoreHandle_t mcp2515_int_sem = NULL;      /* Semaphore for MCP2515 /INT pin */

 /***************SPI HANDLERS**********/

esp_err_t               ESP_ERR;
spi_device_handle_t     spi_handle;
static const char *TAG = "MCP2515_DRIVER";

/****** FORWARD DECLARATIONS ******/

static void mcp2515_task(void *pvParameters);        /*     Owner of MCP2515 read/write  */




/**** SPI INIT *******/

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
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(ESP_HOST, &dev_cfg, &MCP2515_Object->spi);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "SPI BUS INIT AND DEVICE ADDED");
    ESP_LOGI(TAG, "SPI HANDLE: %p \n", MCP2515_Object->spi);

    return true;

}

/*     ISR - MCP2515 \INT PIN INTERRUPT HANDLER    */

 /*
 @brief: all this ISR does is wake up the MCP2515 task when /INT is pulled low
 */
 static void IRAM_ATTR mcp2515_ISR(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  //sets task woken to FALSE
    xSemaphoreGiveFromISR(mcp2515_int_sem, &xHigherPriorityTaskWoken);  //
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**** MCP2515 CAN CHIP INIT ********/

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

    /*  Create the Semaphore before enabling the ISR    */

    mcp2515_int_sem = xSemaphoreCreateBinary();

    /* Set gpio /INT pin as input (PULLED UP) (you sure it's pulled up dude?) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE, //falling edge interrupt
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);        //creates ISR service with flag 0 (default)
    gpio_isr_handler_add(INT_PIN, mcp2515_ISR, NULL);   //attaches ISR handler to INT_PIN

    //at this point the ISR has been enabled, mcp2515_ISR gives semaphore to wake up task every time /INT pulled low

    /*  Finally, create the MCP2515 task with priority 5 (HIGHEST)    */
    xTaskCreate(mcp2515_task, "MCP2515", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "MCP2515 driver successfully initialized...!  ");

 }

 //TODO: define the MCP2515 task logic

 static void mcp2515_task(void *pvParameters) {
    struct can_frame frame_struct;   //actual frame
    CAN_FRAME frame = &frame_struct; //pointer to pass the frame

    while (1) {
        //wait for semaphore from ISR
        if (xSemaphoreTake(mcp2515_int_sem, portMAX_DELAY) == pdTRUE) {
            //Semaphore taken, /INT was pulled low, read message
            ESP_LOGI(TAG, "MCP2515 /INT detected, reading message...");

            if (MCP2515_readMessageAfterStatCheck(frame) == 1) {
                //Successfully read a message
                ESP_LOGI(TAG, "Message received with ID: 0x%08X", (unsigned int long)frame->can_id);
                ESP_LOGI(TAG, "DLC: %d", frame->can_dlc);

                //Create a CanEntry_t to hold the frame and timestamp
                CanEntry_t entry;
                entry.canFrame = frame;
                entry.timestamp = xTaskGetTickCount();

                //Send the entry to the RX queue
                if (xQueueSend(rx_queue, &entry, 0) != pdPASS) {
                    ESP_LOGW(TAG, "RX Queue full, message dropped!");
                } else {
                    ESP_LOGI(TAG, "Message queued successfully.");
                }
            } else {
                ESP_LOGE(TAG, "Failed to read message from MCP2515.");
            }
        }
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

 





