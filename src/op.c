#include "op.h"

#include <assert.h>
#include <ctype.h>

_Static_assert(sizeof(struct op) == OP_SIZE, "unexpected size");

uint8_t
op_get_flag(const struct op* op, char c) {
  uint8_t flag = op->flags;
  char lower = tolower(c);
  if (lower == 'z')
    flag = flag >> 6;
  else if (lower == 'n')
    flag = (flag >> 4) & 3;
  else if (lower == 'h')
    flag = (flag >> 2) & 3;
  else {
    assert(lower == 'c');
    flag &= 3;
  }
  return flag;
}
