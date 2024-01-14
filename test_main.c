#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

static void nop(void** state) {
  (void)state;
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(nop),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
