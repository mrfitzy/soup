#pragma once

#include <stddef.h>
#include <stdint.h>

#define cast_to_intmax_type(value)  ((intmax_t)(value))
#define cast_to_uintmax_type(value) ((uintmax_t)(value))

// cmocka assert macro shims
#define assert_true(c) _shim_assert_true( \
    cast_to_uintmax_type(c), #c, __FILE__, __LINE__)

#define assert_int_equal(a, b) _shim_assert_int_equal( \
    cast_to_intmax_type(a), cast_to_intmax_type(b), __FILE__, __LINE__)

#define assert_memory_equal(a, b, size) _shim_assert_memory_equal( \
    (const void*)(a), (const void*)(b), size, __FILE__, __LINE__)

void _shim_assert_true(
    const uintmax_t result,
    const char* const expression,
    const char* const file,
    const int line);

void _shim_assert_int_equal(
    const intmax_t a,
    const intmax_t b,
    const char* const file,
    const int line);

void _shim_assert_memory_equal(
    const void* const a,
    const void* const b,
    const size_t size,
    const char* const file,
    const int line);

