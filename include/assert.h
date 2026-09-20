#pragma once

#include <stddef.h>
#include <stdint.h>

#define assert(c) ((c) ? (void)0 : zigAssertFail(#c))

#define assert_int_equal(a, b) zigAssertIntEqual((uintptr_t)(a), (uintptr_t)(b))

#define assert_memory_equal(a, b, size) zigAssertMemoryEqual(a, b, size)

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn]] void zigAssertFail(const char* expression);

void zigAssertIntEqual(uintptr_t a, uintptr_t b);

void zigAssertMemoryEqual(const void* a, const void* b, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif
