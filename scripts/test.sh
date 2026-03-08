cmake -S tests -B build/tests
cmake --build build/tests
ctest --test-dir build/tests