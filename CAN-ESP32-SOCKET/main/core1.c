/**
CORE1.C
Purpose: source code for Core 1 functionality - receive messages from TCP client / send over CAN bus
Created: Jan 13 2026
Author: Laith Al-Kinani

*/


 /**
  * CAN BUS NOTES
  * SPI Speed: 2 MHz
  * CAN Speed: 125 KBS
  * CAN Clock: 8 MHz
  */

 #include "core1.h"
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


 canBusContext* canBusMutex = {0};  //initialize context struct