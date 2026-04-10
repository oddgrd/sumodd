#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief A circular buffer that drops the oldest value on push when full.
 *
 * The buffer accepts elements of any type, the size of which needs to be specified at
 * initialization. Data is then memcpy'ed in pop and push, based on the element size.
 */
typedef struct
{
    // The backing buffer.
    uint8_t *data;
    uint32_t capacity;
    // Size of the element to store in bytes.
    uint32_t element_size;
    // Index to write to when pushing onto the queue.
    uint32_t head;
    // Index to read from when poping from the queue.
    uint32_t tail;
} RingBuffer;

/**
 * @brief Initialize a circular buffer.
 *
 * It is up to the caller to provide the backing storage for the ring buffer, which allows the
 * caller to set the size.
 *
 * IMPORTANT: capacity must be a power of two, due to an optimization in wrapping the buffer.
 *
 * @param rb            Pointer to ring buffer to initialize.
 * @param buffer        Backing buffer.
 * @param capacity      Size of the backing buffer in bytes. Must be > 0, and a power of 2.
 * @param element_size  Any type can be used as an element in the buffer, so we must pass its size.
 */
// TODO: assert on capacity param being power of two.
void ring_buffer_init(RingBuffer *rb, uint8_t *buffer, uint32_t capacity, uint32_t element_size);

/**
 * @brief Push an element to the ring buffer.
 *
 * @param rb    Pointer to ring buffer.
 * @param data  Pointer to element to push.
 */
void ring_buffer_push(RingBuffer *rb, const void *element);

/**
 * @brief Pop an element from the ring buffer.
 *
 * @param rb   Pointer to ring buffer.
 * @param out  Pointer to store the popped element.
 *
 * @return true if the element was popped, false if the buffer was empty.
 */
bool ring_buffer_pop(RingBuffer *rb, void *element);

bool ring_buffer_is_empty(const RingBuffer *rb);