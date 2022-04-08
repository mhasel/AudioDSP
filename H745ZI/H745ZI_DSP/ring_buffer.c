// ringbuffer.c, Michael Haselberger
// Description: This file implements a generic ring buffer (FIFO) to be used for data of any type.
//
// Large parts of this code are inspired by various online sources, which are credited/linked below.
// I took the liberty to take what I liked from each of these and then adapted the code to fit my personal requirements.
// http://www.simplyembedded.org/tutorials/interrupt-free-ring-buffer/ 
// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
// https://www.embedded.com/ring-buffer-basics/

#include <string.h>
#include "ring_buffer.h"

// Maximum amount of ring buffers available. Default value: 1
#define RING_BUFFER_MAX (1)
// Error codes
#define SUCCESS (0)
#define FAILURE (-1)

// Local buffer struct that keeps track of head and tail position internally. This information does not need to be exposed to the handle.
// Head and tail are both declared as volatile. 
// This is because they might be accessed from both the application context and the interrupt context.
struct ring_buffer
{
    size_t element_size;
    size_t max_elements;
    uint8_t* buf;
    volatile size_t head;
    volatile size_t tail;
};

// This structure is allocated as an array private to this file. (static variables in C are "private" to the module they are defined in)
// The maximum number of ring buffers available in the system is determined at compile time by the hash define RING_BUFFER MAX
static struct ring_buffer _rb[RING_BUFFER_MAX];

/******************************************************************************
* Function Name: ring_buffer_init
*******************************************************************************
* Summary:
*  This function is used to initialize the ring buffer
*
* Parameters:
*  1. rb_designator_t* rbd - ring buffer designator, used to index and track the amount of ring buffers
*  2. rb_handle_t* attr - struct which contains information about the size of the buffer, its address in memory and which data it will hold
* Return:
*  SUCCESS - All the programming steps executed successfully
*  FAILURE - Initialization fails in any one of the programming steps
*
******************************************************************************/
int ring_buffer_init(rb_designator_t* rbd, rb_handle_t* attr)
{
    // This static variable counts the number of used ring buffers
    static int index = 0;

    // Check that there is a free ring buffer, and that the rbd and attr pointers are not NULL.
    if ((index < RING_BUFFER_MAX) && (rbd != NULL) && (attr != NULL))
    {
        // Verify that the element size and buffer pointer are both valid
        if ((attr->buffer != NULL) && (attr->element_size > 0))
        {
            /*
             * Check that the size of the ring buffer is a power of 2.
             * Any value which is a power of two will have only one '1' in it's binary representation
             * Example: 10000(16) - 1 = 01111(15): 10000 & 01111 == 0
             * This allows for more efficient masking instead of expensive modulo operations to maintain itself.
            */
            if (((attr->max_elements - 1) & attr->max_elements) == 0)
            {
                // Initialize the ring buffer internal variables
                _rb[index].head = 0;
                _rb[index].tail = 0;
                _rb[index].buf = attr->buffer;
                _rb[index].element_size = attr->element_size;
                _rb[index].max_elements = attr->max_elements;

                // Index is passed back to the caller as the ring buffer descriptor.
                // The variable index is also incremented to indicate the ring buffer is used.
                *rbd = index++;
                return SUCCESS;
            }
        }
    }

    return FAILURE;
}

// ------------------------Static helper functions---------------------------------------
/*
    Both calculate the difference between the head and the tail and then compare the result against the number of elements or zero respectively.
    Notice that in the subsequent functions, the head and tail are not wrapped within the bounds of the ring buffer.
    Instead they are incremented and wrap around automatically when they overflow (unsigned)
*/

/******************************************************************************
* Function Name: _ring_buffer_full
*******************************************************************************
* Summary:
*  This static function is used to determine if the ring buffer is currently full.
*
* Parameters:
*  1. struct ring_buffer* rb - Pointer to the ring buffer that is to be checked.
* Return:
*  1 - The buffer is full
*  0 - The buffer is not full
*
******************************************************************************/
static int _ring_buffer_full(struct ring_buffer* rb)
{
    return ((rb->head - rb->tail) == rb->max_elements) ? 1 : 0;
}

/******************************************************************************
* Function Name: _ring_buffer_empty
*******************************************************************************
* Summary:
*  This static function is used to determine if the ring buffer is currently empty.
*
* Parameters:
*  1. struct ringbuffer* rb - Pointer to the ring buffer that is to be checked.
* Return:
*  1 - The buffer is empty
*  0 - The buffer is not empty
*
******************************************************************************/
static int _ring_buffer_empty(struct ring_buffer* rb)
{
    return ((rb->head - rb->tail) == 0U) ? 1 : 0;
}

/******************************************************************************
* Function Name: ring_buffer_count
*******************************************************************************
* Summary:
*  This function gets the current amount of items in buffer.
*
* Parameters:
*  1. rb_designator_t* rbd - ring buffer designator, used to index and track the amount of ring buffers
* Return:
*  The current amount of items in the buffer
*
******************************************************************************/
int ring_buffer_count(rb_designator_t rbd)
{
    return (_rb[rbd].head - _rb[rbd].tail);
}

/******************************************************************************
* Function Name: ring_buffer_put
*******************************************************************************
* Summary:
*  This function adds element into ring buffer.
*
* Parameters:
*  1. rb_designator_t* rbd - ring buffer designator, used to index and track the amount of ring buffers
* Return:
*  SUCCESS - Data was successfully added into the buffer.
*  FAILURE - Unable to add data into buffer.
*
******************************************************************************/
int ring_buffer_put(rb_designator_t rbd, const void* data)
{
    // Validate argument and check if ring buffer is full
    if ((rbd < RING_BUFFER_MAX) && (_ring_buffer_full(&_rb[rbd]) == 0))
    {
        /*
         * Buffer is just an array of bytes, so in order to copy the data to the correct location, find out where free elements start in memory.
         * This is usually done via modulo, but since buffer size is restricted to power of 2, it can be done via binary operation.
         * Subtracting one from any power of two results in a binary string of ones. Logical ANDing the result with any value will obtain the modulus.
         * Size of data is defined by the caller, so the actual byte offset into the memory array is calculated by taking the element and multiplying it
         * by the size of each element in bytes.
        */
        const size_t offset = (_rb[rbd].head & (_rb[rbd].max_elements - 1)) * _rb[rbd].element_size;

        // The memcpy function is used to copy a block of data from a source address to a destination address.
        // void * memcpy(void * destination, const void * source, size_t num)
        memcpy(&(_rb[rbd].buf[offset]), data, _rb[rbd].element_size);
        // Update position of the head pointer.
        _rb[rbd].head++;

        return SUCCESS;
    }

    return FAILURE;
}

/******************************************************************************
* Function Name: ring_buffer_get
*******************************************************************************
* Summary:
*  This function writes first element from ring buffer into passed pointer and then removes it from the buffer.
*
* Parameters:
*  1. rb_designator_t* rbd - ring buffer designator, used to index and track the amount of ring buffers
* Return:
*  SUCCESS - Data was successfully read and removed from the buffer.
*  FAILURE - Unable to get data from buffer.
*
******************************************************************************/
int ring_buffer_get(rb_designator_t rbd, void* data)
{
    // Essentially the same as ring_buffer_put, but instead of putting data into the buffer at the head, data is taken out at the tail.
    if ((rbd < RING_BUFFER_MAX) && (_ring_buffer_empty(&_rb[rbd]) == 0))
    {
        const size_t offset = (_rb[rbd].tail & (_rb[rbd].max_elements - 1)) * _rb[rbd].element_size;
        memcpy(data, &(_rb[rbd].buf[offset]), _rb[rbd].element_size);
//		// set value taken from ring buffer to 0
//		memset(&(_rb[rbd].buf[offset]), 0, _rb[rbd].element_size);
        _rb[rbd].tail++;

        return SUCCESS;
    }

    return FAILURE;
}

/******************************************************************************
* Function Name: ring_buffer_clear
*******************************************************************************
* Summary:
*  This function overwrites each element in the buffer with 0, essentially clearing the buffer.
*
* Parameters:
*  1. rb_designator_t* rbd - ring buffer designator, used to index and track the amount of ring buffers
* Return:
*  None.
*
******************************************************************************/
void ring_buffer_clear(rb_designator_t rbd)
{
    memset(_rb[rbd].buf, 0, (_rb[rbd].max_elements * _rb[rbd].element_size));
    _rb[rbd].head = 0;
    _rb[rbd].tail = 0;
}