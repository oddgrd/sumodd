#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer *rb, uint8_t *buffer, size_t capacity)
{
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->data = buffer;
};

bool ring_buffer_push(ring_buffer *rb, uint8_t data)
{
    if (rb->count == rb->capacity)
    {
        return false;
    }
    rb->data[rb->head] = data;
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;

    return false;
};

bool ring_buffer_pop(ring_buffer *rb, uint8_t *out)
{
    if (rb->count == 0)
    {
        return false;
    }
    *out = rb->data[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;

    return true;
};

bool ring_buffer_is_empty(const ring_buffer *rb)
{
    return rb->count == 0;
};