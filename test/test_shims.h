#pragma once

#include <stddef.h>
#include <stdint.h>

#define assert_int_equal(a, b) zig_assert_int_equal(a, b)

#define assert_true(c) zig_assert_true(c, #c)

#define assert_memory_equal(a, b, size) zig_assert_memory_equal(a, b, size)

void zig_assert_int_equal(int a, int b);

void zig_assert_true(int result, const char* expression);

void zig_assert_memory_equal(const void* a, const void* b, size_t size);
