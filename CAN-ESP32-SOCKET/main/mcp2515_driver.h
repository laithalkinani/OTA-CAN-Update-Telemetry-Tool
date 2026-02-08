#ifndef MCP2515_DRIVER_H
#define MCP2515_DRIVER_H


/**
 * 
 * MCP2515_DRIVER.h
 * Purpose: define all GPIO locations and settings for SPI and CAN functions
 * 
 **/ 

 #include "can.h"
 #include <stdbool.h>


/*****************SPI MACROS********************/


#define     SCK_PIN     18
#define     MOSI_PIN    23
#define     MISO_PIN    19
#define     CS_PIN      5
#define     INT_PIN     4
#define     LED         2   //for debugging

#define     ESP_HOST    VSPI_HOST


/*******TIMESTAMPED CAN FRAME STRUCT********/

typedef struct 
{
    CAN_FRAME canFrame;     
    uint32_t timestamp;

} CanEntry_t;       //timestamped CAN entry

//TODO: align this to fit memory efficiently



/***********SPI FUNCTION PROTOTYPES**********/

bool SPI_Init(void);


/***********CAN FUNCTION PROTOTYPES**********/

void CAN_Init(void);
void mcp2515_task(void *pvParameters);        /*     Owner of MCP2515 read/write  */




#endif //MCP2515_DRIVER_H