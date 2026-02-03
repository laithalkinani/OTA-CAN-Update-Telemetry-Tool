/*
ringbuffer for canentry_t data
*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H


#include "can_stuff.h"
#include "can.h"
#include <stdint.h>

#define RINGBUFFER_SIZE ((uint16_t)64)

typedef struct 
{
    CanEntry_t canEntry[RINGBUFFER_SIZE];       //data of type CanEntry_t
    uint16_t size;
    uint16_t head;
    uint16_t tail;
    uint16_t bufferLen;             //for wraparound


} ringbuffer_t;

//TODO: enforce power of two

/*
@brief: returns a ringbuffer_t* handle
@params: ringbuffer object
@ret: none
*/
void ringbuffer_init(ringbuffer_t* ringbuffer);

/*
@brief: queues a CanEntry_t frame to the HEAD
@param: ringbuffer_t* ringbuffer handler
@ret: none
*/

void ringbuffer_queue(ringbuffer_t* ringbuffer);

/*
@brief: checks if HEAD is equal to SIZE - 1
@param: ringbuffer_t* ringbuffer handler
@ret: uint8_t 0 for not full, 1 for full
*/
uint8_t isRingBufferFull(ringbuffer_t* ringbuffer);


/*
@brief: copies the entire contents of canRxBuffer to networkTxBuffer
@param: the two buffers
@ret: none
*/
void ringbuffer_copy(ringbuffer_t* canRxBuffer, ringbuffer_t* networkTxBuffer);



/*
@brief: returns the current HEAD index and the value of the canEntry into a variable
@params: ringbuffer_t* handler, CanEntry_t* canEntry frame variable
@ret: uint8_t current index of HEAD
*/
uint8_t ringbuffer_peek(ringbuffer_t* ringbuffer, CanEntry_t* canEntry);


#endif //RINGBUFFER_H