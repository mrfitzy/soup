#pragma once

#include <stddef.h>
#include <stdint.h>

#define assert_int_equal(a, b) zig_assert_int_equal((uintptr_t)(a), (uintptr_t)(b))

#define assert(c) zig_assert_true((uintptr_t)(c), #c)

#define assert_true(c) zig_assert_true((uintptr_t)(c), #c)

#define assert_memory_equal(a, b, size) zig_assert_memory_equal(a, b, size)

#ifdef __cplusplus
extern "C" {
#endif

void zig_assert_int_equal(uintptr_t a, uintptr_t b);

void zig_assert_true(uintptr_t result, const char* expression);

void zig_assert_memory_equal(const void* a, const void* b, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif
