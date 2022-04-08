// ringbuffer.h, Michael Haselberger
// Description: This file contains declarations and macro definitions for a ringbuffer, implemented in ringbuffer.c

/* ========================================
 * Non-standard preprocessor directive designed to cause the current source file to be included only once in a single compilation.
 * Not necessarily available in all compilers. Serves the same purpose as header guards, but is faster.
*/ 
// #pragma once
/*
 * Header guard
 * https://www.learncpp.com/cpp-tutorial/header-guards/
 * ========================================
*/
#ifndef __RINGBUFFERAPI_H__
#define __RINGBUFFERAPI_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>

// Macro to check size of array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Ring buffer descriptor
// This descriptor will be used by the caller to access the ring buffer which it has initialized. 
// It is an unsigned integer type because it will be used as an index into an array of the internal ringBuffer structure.
typedef uint8_t rb_designator_t;

/*  -----------------------------------------------------------------------------------------------------------------------------
*   This structure contains the user defined attributes of the ring buffer which will be passed into the initialization routine.
*   The design of this structure means that the user must provide the memory used by the ring buffer to store the data.
*   This is required because it is commonly considered bad practice to use dynamic memory allocation in embedded systems (i.e. malloc, realloc, calloc, etc�).
*
*   Members:
*   element_size:       The size of each element.
*   max_elements:       The number of elements.
*   buffer:             A pointer to the buffer which will hold the data.
*   -----------------------------------------------------------------------------------------------------------------------------
*/    
typedef struct 
{
    size_t element_size;
    size_t max_elements;
    void* buffer;
} rb_handle_t;

int ring_buffer_init(rb_designator_t* rbd, rb_handle_t* attr);
int ring_buffer_put(rb_designator_t rbd, const void* data);
int ring_buffer_get(rb_designator_t rbd, void* data);
int ring_buffer_count(rb_designator_t rbd);
void ring_buffer_clear(rb_designator_t rbd);
	
#ifdef __cplusplus
}
#endif
#endif
