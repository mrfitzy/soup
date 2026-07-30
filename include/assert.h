#pragma once

#include <stddef.h>
#include <stdint.h>

#define assert_int_equal(a, b) zigAssertIntEqual((uintptr_t)(a), (uintptr_t)(b))

#define assert(c) zigAssertTrue((uintptr_t)(c), #c)

#define assert_true(c) zigAssertTrue((uintptr_t)(c), #c)

#define assert_memory_equal(a, b, size) zigAssertMemoryEqual(a, b, size)

#ifdef __cplusplus
extern "C" {
#endif

void zigAssertIntEqual(uintptr_t a, uintptr_t b);

void zigAssertTrue(uintptr_t result, const char* expression);

void zigAssertMemoryEqual(const void* a, const void* b, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif
