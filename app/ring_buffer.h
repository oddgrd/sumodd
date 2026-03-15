#pragma once

#include <stdbool.h>
#include <stdint.h>

// TODO: refactor implementation so we can use the ring buffer for various data types, not just
// bytes.

/**
 * @brief A circular buffer of bytes that disallows writes when full.
 */
typedef struct
{
    // The backing buffer.
    uint8_t *data;
    uint32_t capacity;
    uint32_t head;
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
 * @param rb        Pointer to ring buffer to initialize.
 * @param buffer    Backing buffer.
 * @param capacity  Size of the backing buffer in bytes. Must be > 0, and a power of 2.
 */
// TODO: assert on capacity param being power of two.
void ring_buffer_init(RingBuffer *rb, uint8_t *buffer, uint32_t capacity);

/**
 * @brief Push an item to the ring buffer.
 *
 * @param rb    Pointer to ring buffer.
 * @param data  Item to push.
 *
 * @return true if the item was pushed, false if the buffer was full.
 */
bool ring_buffer_push(RingBuffer *rb, uint8_t data);

/**
 * @brief Pop an item from the ring buffer.
 *
 * @param rb   Pointer to ring buffer.
 * @param out  Pointer to store the popped item.
 *
 * @return true if the item was popped, false if the buffer was empty.
 */
bool ring_buffer_pop(RingBuffer *rb, uint8_t *out);

bool ring_buffer_is_empty(const RingBuffer *rb);