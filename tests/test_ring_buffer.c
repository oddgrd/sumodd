#include "unity.h"
#include "ring_buffer.h"

#define TEST_BUFFER_CAPACITY 8

static RingBuffer rb;
static uint8_t buffer[TEST_BUFFER_CAPACITY];

void setUp(void)
{
    ring_buffer_init(&rb, buffer, 8, sizeof(uint8_t));
}

void tearDown(void)
{
}

void test_ring_buffer_is_empty_after_init(void)
{
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

void test_push_then_pop_then_empty(void)
{
    uint8_t item = 42;
    TEST_ASSERT_TRUE(ring_buffer_push(&rb, &item));

    uint8_t out = 0;
    TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(42, out);
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

void test_push_until_full(void)
{
    // Note that the capacity is one smaller, since we reserve one slot to determine if the buffer is full.
    for (int i = 0; i < TEST_BUFFER_CAPACITY - 1; i++)
    {
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, &i));
    }
    uint8_t item = 9;
    TEST_ASSERT_FALSE(ring_buffer_push(&rb, &item));
}

typedef struct
{
    uint8_t type;
    uint8_t payload;
} test_event_t;

void test_struct_elements(void)
{
    static test_event_t event_buffer[8];
    RingBuffer event_rb;
    ring_buffer_init(&event_rb, (uint8_t *)event_buffer, 8, sizeof(test_event_t));

    test_event_t in = {.type = 3, .payload = 42};
    TEST_ASSERT_TRUE(ring_buffer_push(&event_rb, &in));

    test_event_t out = {0};
    TEST_ASSERT_TRUE(ring_buffer_pop(&event_rb, &out));
    TEST_ASSERT_EQUAL_UINT8(3, out.type);
    TEST_ASSERT_EQUAL_UINT8(42, out.payload);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ring_buffer_is_empty_after_init);
    RUN_TEST(test_push_then_pop_then_empty);
    RUN_TEST(test_push_until_full);
    RUN_TEST(test_struct_elements);

    return UNITY_END();
}