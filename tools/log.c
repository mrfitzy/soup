#include "log.h"

#include "assert.h"

#include <stdarg.h>
#include <stdio.h>

void zigLogDebug(const char*);
void zigLogError(const char*);

static inline void
log(void (*log_fn)(const char*), const char* fmt, va_list ap) {
  char buf[128];
  int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  assert(ret >= 0);
  assert((size_t)ret < sizeof(buf));
  log_fn(buf);
}

void
log_debug(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  log(zigLogDebug, fmt, ap);
  va_end(ap);
}

void
log_error(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  log(zigLogError, fmt, ap);
  va_end(ap);
}
