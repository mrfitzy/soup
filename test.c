#include "test.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

int
test_run(void (*test_func)(void**)) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_func),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
