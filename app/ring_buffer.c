#include <stdint.h>
#include <stdbool.h>

#include "ring_buffer.h"

void ring_buffer_init(RingBuffer *rb, uint8_t *buffer, uint32_t capacity)
{
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->data = buffer;
}

// TODO: UNSAFE! Disable IRQ at start of function, restore after, to ensure single producer.// Ring buffer logic:
bool ring_buffer_push(RingBuffer *rb, uint8_t data)
{
    uint32_t head = rb->head;

    // TODO: better comment, move to header file?
    // Since the buffer wraps, we cannot just check if (head - tail) == capacity. And we don't want
    // to track a count of elements, because that is unsafe, it would need to be mutated by both
    // producers and consumers.
    //
    // We also cannot check if head == tail to determine if the queue is full, because that is what
    // we do to check for empty.
    // Therefore, we check for empty with head == tail, and we check for full with
    // (head + 1) & (rb->capacity - 1), checking if another write would make head == tail.
    // When the buffer is full, deny new writes. When new reads are made, the queue will no longer
    // be full, and accept writes again.
    uint32_t next_head = (head + 1) & (rb->capacity - 1);

    if (next_head == rb->tail) // Full
    {
        return false; // Drop or handle overflow
    }

    rb->data[head] = data;

    rb->head = next_head;
    return true;
}

// Called from main loop only
bool ring_buffer_pop(RingBuffer *rb, uint8_t *data)
{
    uint32_t tail = rb->tail;

    if (tail == rb->head) // Empty — head read once, safe
    {
        return false;
    }

    // TODO: consider compiler barrier? To ensure read happens before tail advances.
    *data = rb->data[tail];

    // Equivalent to modulo as long as the capacity is a power of two.
    // Examples with capacity 8:
    // (3 + 1) & (8 - 1)
    // 00000100 & 00000111 = 00000100 // no wrap, set to 4
    // -----------------
    // (7 + 1) & (8 - 1)
    // 00001000 & 00000111 = 00000000 // reached capacity, wrap to 0
    rb->tail = (tail + 1) & (rb->capacity - 1);
    return true;
}

bool ring_buffer_is_empty(const RingBuffer *rb)
{
    return rb->tail == rb->head;
}
