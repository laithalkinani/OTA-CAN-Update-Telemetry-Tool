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
 #include "can_stuff.h"
 #include "can.h"
 #include "mcp2515.h"
 #include "driver/spi_master.h"
 #include "driver/gpio.h"
 #include "esp_err.h"

 /***************SPI HANDLERS**********/

esp_err_t               ESP_ERR;
spi_device_handle_t     spi_handle;


/****SPI FUNCTION DEFINITIONS****/

 void SPI_Init(void)
 {
    spi_bus_config_t buscfg = {
                    .mosi_io_num = MOSI_PIN,
                    .miso_io_num = MISO_PIN,
                    .sclk_io_num = SCK_PIN,
                    .quadhd_io_num = -1,
                    .quadwp_io_num = -1

    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 2000000, //2 MHz
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = CS_PIN,
        .queue_size = 3
    };

    ESP_ERR = spi_bus_initialize(ESP_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ESP_ERR == ESP_OK);
    ESP_ERR = spi_bus_add_device(ESP_HOST, &devcfg, &spi_handle);
    assert(ESP_ERR == ESP_OK);


 }

 void CAN_Init(void)
 {
    MCP2515_init();
    SPI_Init();
    MCP2515_reset();
    MCP2515_setBitrate(CAN_125KBPS, MCP_8MHZ); //125 KBS bit rate
    MCP2515_setNormalMode();

    //Mask/Filtering: can make this another function later
    MCP2515_setFilterMask(MASK0, false, 0x000);  // accept everything
    MCP2515_setFilterMask(MASK1, false, 0x000); //for now just for testing


 }

 /**
  * Procedure: Poll the GPIO INT_PIN input pin to check if the /INT pin on the MCP2515
  * has been pulled low, indicating a frame has landed in its RXBUFFER.
  * Note: the ESP32 documentation actually says polling is faster than its ISR 
  * 
  */

 
  void CAN_Polling(void *arg)
  {
    //The naming is soo confusing here ngl 
    struct can_frame frame_struct;   //actual frame
    CAN_FRAME frame = &frame_struct; //pointer to pass the frame

    CAN_Init();

    while(1) //TODO: be careful having a while(1) loop here... 
    //Because what if you have a while(1) loop running for the wifi stuff?
    //Now you have to use both cores.
    {

        if (gpio_get_level(INT_PIN) == 0) 
        {
            if (MCP2515_readMessageAfterStatCheck(frame) == ERROR_OK)
            {
                printf("CAN ID: 0x%X, DLC: %d, Data: ", (uint8_t)frame->can_id, frame->can_dlc);
                char str[frame->can_dlc + 1]; //length of data + null terminator
                for (uint8_t i = 0; i < frame->can_dlc; i++)
                {
                    str[i] = frame->data[i];
                }
                str[frame->can_dlc] = '\0'; //null terminate it

                printf("\n Received: %s\n", str);

            }
        }

  }
}
 





