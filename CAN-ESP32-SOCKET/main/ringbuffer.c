

#include "ringbuffer.h"
#include "mcp2515.h"
#include "esp_log_timestamp.h"
#include <stdint.h>


void ringbuffer_init(ringbuffer_t* ringbuffer)
{
    ringbuffer->size = RINGBUFFER_SIZE;
    ringbuffer->head = 0;
    ringbuffer->tail = 0;
    ringbuffer->bufferLen = (ringbuffer->size) - 1;
    
    static const CanEntry_t zeroEntry = {0};      //immutable zero struct for fast zeroing assignment

    for (uint8_t i = 0; i < RINGBUFFER_SIZE; i++)
    {
        ringbuffer->canEntry[i] = zeroEntry;        //zero out each canEntry
    }


}

void ringbuffer_queue(ringbuffer_t* ringbuffer)
{
    /*  retrieve CAN_FRAME struct inside head   */

    MCP2515_readMessageAfterStatCheck(ringbuffer->canEntry[ringbuffer->head].canFrame);

    /*  timestamp the CAN message with uint32_t system timer   */

    ringbuffer->canEntry[ringbuffer->head].timestamp = esp_log_timestamp();

    /*  advance the HEAD pointer with wraparound  */

    ringbuffer->head = (ringbuffer->head + 1) & (ringbuffer->bufferLen);

}



uint8_t isRingBufferFull(ringbuffer_t* ringbuffer)
{
    return (ringbuffer->head == ringbuffer->bufferLen) ? 1 : 0;

}

void ringbuffer_copy(ringbuffer_t* canRxBuffer, ringbuffer_t* networkTxBuffer)
{
    /*  copy all contents from 0 to HEAD to the other buffer    */

    for (uint16_t i = 0; i <= (canRxBuffer->bufferLen); i++)
    {
        networkTxBuffer->canEntry[i] = canRxBuffer->canEntry[i];
    }

    /*  either bring head back to 0 or advance tail, not sure which one to do yet */
    //TODO
}


uint8_t ringbuffer_peek(ringbuffer_t* ringbuffer, CanEntry_t* canEntry)
{
    canEntry = &(ringbuffer->canEntry[ringbuffer->head]);       //is it correct to deref it here?

    return (ringbuffer->head);
}