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
  log_fn(buf);
  if ((ret < 0) || ((size_t)ret >= sizeof(buf))) {
    (void) snprintf(
        buf, sizeof(buf), "log: previous log truncated (error %d)", ret);
    zigLogError(buf);
  }
}

void
log_debug_impl(const char* fmt, ...) {
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

[[noreturn]] void fatal(const char* fmt, ...) {
  log_error("fatal error encountered:");
  va_list ap;
  va_start(ap, fmt);
  log(zigLogError, fmt, ap);
  va_end(ap);
  assert(false);
}
