/*

COREO.H
Purpose: header file for Core 0 functionality - receiving from CAN bus
Created: Jan 13 2026
Author: Laith Al-Kinani
*/

#ifndef CORE0_H
#define CORE0_H

#include <stdint.h>



typedef enum
{

    CORE0_STATE0,
    CORE0_STATE1,
    CORE0_STATE2    

} Core0States_t;

typedef struct
{
    uint8_t currentState;
    uint8_t nextState;

} currentCore0State;





#endif //COREO_H