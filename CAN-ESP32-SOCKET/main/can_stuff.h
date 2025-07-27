#ifndef CAN_STUFF_H
#define CAN_STUFF_H

/**
 * 
 * can_stuff_h
 * Purpose: define all GPIO locations and settings for SPI and CAN functions
 * 
 **/ 



/*****************SPI MACROS********************/


#define     SCK_PIN     13
#define     MOSI_PIN    14
#define     MISO_PIN    27
#define     CS_PIN      26
#define     INT_PIN     25
#define     LED         2   //for debugging

#define     ESP_HOST    HSPI_HOST


/***********SPI FUNCTION PROTOTYPES**********/

bool SPI_Init(void);


/***********CAN FUNCTION PROTOTYPES**********/

void CAN_Init(void);
void CAN_Polling(void* arg);



#endif //CAN_STUFF_H