#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief A circular buffer of bytes that disallows writes when full.
 */
typedef struct
{
    // The backing buffer.
    uint8_t *data;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
} ring_buffer;

/**
 * @brief Initialize a circular buffer.
 *
 * It is up to the caller to provide the backing storage for the ring buffer, which allows the
 * caller to set the size.
 *
 * @param rb        Pointer to ring buffer to initialize.
 * @param buffer    Backing buffer.
 * @param capacity  Size of the backing buffer in bytes. Must be > 0.
 */
void ring_buffer_init(ring_buffer *rb, uint8_t *buffer, size_t capacity);

/**
 * @brief Push an item to the ring buffer.
 *
 * @param rb    Pointer to ring buffer.
 * @param data  Item to push.
 *
 * @return true if the item was pushed, false if the buffer was full.
 */
bool ring_buffer_push(ring_buffer *rb, uint8_t data);

/**
 * @brief Pop an item from the ring buffer.
 *
 * @param rb   Pointer to ring buffer.
 * @param out  Pointer to store the popped item.
 *
 * @return true if the item was popped, false if the buffer was empty.
 */
bool ring_buffer_pop(ring_buffer *rb, uint8_t *out);

bool ring_buffer_is_empty(const ring_buffer *rb);