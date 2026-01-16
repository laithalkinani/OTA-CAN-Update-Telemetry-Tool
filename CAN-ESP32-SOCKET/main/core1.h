/*

CORE1.H
Purpose: header file for Core 1 functionality - sending to CAN bus
Created: Jan 13 2026
Author: Laith Al-Kinani
*/

#ifndef CORE1_H
#define CORE1_H

#include <stdint.h>

typedef struct
{
    uint8_t isBusLocked;

} canBusContext;

typedef enum
{
    CORE1_STATE0,
    CORE1_STATE1,
    CORE1_STATE2

} Core1States;



#endif //CORE1_H