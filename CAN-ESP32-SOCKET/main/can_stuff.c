/**
 * can_stuff.c
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
 #include "can_stuff.h"
 #include "can.h"
 #include "mcp2515.h"
 #include "driver/spi_master.h"
 #include "driver/gpio.h"
 #include "esp_err.h"
 #include "esp_log.h"

 /***************SPI HANDLERS**********/

esp_err_t               ESP_ERR;
spi_device_handle_t     spi_handle;
static const char *TAG = "CAN_STUFF";



/****SPI FUNCTION DEFINITIONS****/

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


 void CAN_Init(void)
 {

       // Initialize LED GPIO for debugging
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    // Blink LED 3 times to confirm board is alive
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    MCP2515_init();
    SPI_Init();
    MCP2515_reset();
    MCP2515_setBitrate(CAN_125KBPS, MCP_8MHZ); //125 KBS bit rate
    MCP2515_setNormalMode();

    //Mask/Filtering: can make this another function later
    MCP2515_setFilterMask(MASK0, false, 0x000);  // accept everything
    MCP2515_setFilterMask(MASK1, false, 0x000); //for now just for testing

    MCP2515_setFilter(0, false, 0x000);  // Accept all
    MCP2515_setFilter(1, false, 0x000);
    MCP2515_setFilter(2, false, 0x000);
    MCP2515_setFilter(3, false, 0x000);
    MCP2515_setFilter(4, false, 0x000);
    MCP2515_setFilter(5, false, 0x000);

    //try this


    //set gpio /INT pin as input (PULLED UP)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);


 }

 /**
  * Procedure: Poll the GPIO INT_PIN input pin to check if the /INT pin on the MCP2515
  * has been pulled low, indicating a frame has landed in its RXBUFFER.
  * Note: the ESP32 documentation actually says polling is faster than its ISR 
  * 
  */

 
  void CAN_Polling(void *arg)
  {
    // //The naming is soo confusing here ngl 
    struct can_frame frame_struct;   //actual frame
    CAN_FRAME frame = &frame_struct; //pointer to pass the frame
    CAN_Init();
    ESP_LOGI(TAG, "GET STATUS: 0x%02X", MCP2515_getStatus());
    // MCP2515_reset();
    ESP_LOGI(TAG, "SECOND GET STATUS: 0x%02X", MCP2515_getStatus());

    

    while(1) //TODO: be careful having a while(1) loop here... 
    //Because what if you have a while(1) loop running for the wifi stuff?
    //Now you have to use both cores.
    {
        //    ESP_LOGI(TAG, "THIS WORKS %p", MCP2515_Object->spi);

        if (gpio_get_level(INT_PIN) == 0) 
        {
            ESP_LOGI(TAG, "INT pin pulled low, checking for CAN frame...");
            if (MCP2515_readMessageAfterStatCheck(frame) == ERROR_OK)
            {
                ESP_LOGI(TAG,"CAN ID: 0x%X, DLC: %d, Data: ", (uint8_t)frame->can_id, frame->can_dlc);
                char str[frame->can_dlc + 1]; //length of data + null terminator
                for (uint8_t i = 0; i < frame->can_dlc; i++)
                {
                    str[i] = frame->data[i];
                }
                str[frame->can_dlc] = '\0'; //null terminate it

                ESP_LOGI(TAG,"\n Received: %s\n", str);

            }
            else 
            {
                ESP_LOGI(TAG, "ERROR NOT OK?");
            }
        }

        else 
        {
            ESP_LOGI(TAG, "INT NEVER PULLED LOW");
        }

        vTaskDelay(pdMS_TO_TICKS(10)); //delay for polling.

  }
}
 





