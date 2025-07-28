#ifndef CAN_STUFF_H
#define CAN_STUFF_H

/**
 * 
 * can_stuff_h
 * Purpose: define all GPIO locations and settings for SPI and CAN functions
 * 
 **/ 



/*****************SPI MACROS********************/


#define     SCK_PIN     18
#define     MOSI_PIN    23
#define     MISO_PIN    19
#define     CS_PIN      5
#define     INT_PIN     4
#define     LED         2   //for debugging

#define     ESP_HOST    VSPI_HOST


/***********SPI FUNCTION PROTOTYPES**********/

bool SPI_Init(void);


/***********CAN FUNCTION PROTOTYPES**********/

void CAN_Init(void);
void CAN_Polling(void* arg);



#endif //CAN_STUFF_H