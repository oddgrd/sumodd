#include "unity.h"
#include "ring_buffer.h"

#define TEST_BUFFER_CAPACITY 8

static RingBuffer rb;
static uint8_t buffer[TEST_BUFFER_CAPACITY];

void setUp(void)
{
    ring_buffer_init(&rb, buffer, 8);
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
    TEST_ASSERT_TRUE(ring_buffer_push(&rb, 42));

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
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, i));
    }
    TEST_ASSERT_FALSE(ring_buffer_push(&rb, 9));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ring_buffer_is_empty_after_init);
    RUN_TEST(test_push_then_pop_then_empty);
    RUN_TEST(test_push_until_full);

    return UNITY_END();
}