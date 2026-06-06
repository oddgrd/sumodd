cmake_minimum_required(VERSION 3.22)

project(sumo_tests C)

set(CMAKE_C_STANDARD 11)

enable_testing()

add_executable(ring_buffer_tests
    test_ring_buffer.c
    unity/unity.c
    ../app/ring_buffer.c
)

target_include_directories(ring_buffer_tests PRIVATE
    unity
    ../app
)

# Strong warnings
target_compile_options(ring_buffer_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Sanitizers
target_compile_options(ring_buffer_tests PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
)

target_link_options(ring_buffer_tests PRIVATE
    -fsanitize=address,undefined
)

add_test(NAME ring_buffer_tests COMMAND ring_buffer_tests)