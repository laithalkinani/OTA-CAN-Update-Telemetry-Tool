
#include "can_stuff.h"
#include "can.h"
#include <stdint.h>

typedef struct 
{
    CanEntry_t* canEntry;        //data of type CanEntry_t
    uint8_t isEmpty;
    uint8_t isFull;
    uint16_t size;
    uint16_t head;
    uint16_t tail;


} ringbuffer_t;

//TODO: enforce power of two

/*
@brief: returns a new ringbuffer_t object zeroed out
@params: size of ringbuffer
@ret: ringbuffer_t struct
*/
ringbuffer_t ringbuffer_init(const uint16_t size);
