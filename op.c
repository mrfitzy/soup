#include "op.h"

#include <assert.h>

uint8_t
op_get_flag(const struct op* op, char c) {
  uint8_t flag = op->flags;
  if (c == 'Z')
    flag = flag >> 6;
  else if (c == 'N')
    flag = (flag >> 4) & 3;
  else if (c == 'H')
    flag = (flag >> 2) & 3;
  else {
    assert(c == 'C');
    flag &= 3;
  }
  return flag;
}
