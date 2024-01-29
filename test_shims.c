#include "diag.h"

#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

void
_shim_assert_true(
    const uintmax_t result,
    const char* const expression,
    const char* const file,
    const int line) {
  if (!result)
    print_backtrace();
  _assert_true(result, expression, file, line);
}

void
_shim_assert_int_equal(
    const intmax_t a,
    const intmax_t b,
    const char* const file,
    const int line) {
  if (a != b)
    print_backtrace();
  _assert_int_equal(a, b, file, line);
}

void
_shim_assert_memory_equal(
    const void* const a,
    const void* const b,
    const size_t size,
    const char* const file,
    const int line) {
  if (memcmp(a, b, size))
    print_backtrace();
  _assert_memory_equal(a, b, size, file, line);
}

