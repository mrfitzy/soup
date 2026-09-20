#pragma once

#define LOG_LEVEL LOG_LEVEL_INFO

#define LOG_LEVEL_ERR   0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

static inline int touch_sink_(int first, ...) { (void)first; return 0; }
#define TOUCH(...) ((void)sizeof(touch_sink_(0 __VA_OPT__(,) __VA_ARGS__)))

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
void log_debug_impl(const char* fmt, ...);
#define log_debug(fmt, ...) log_debug_impl(fmt, __VA_ARGS__)
#else
#define log_debug(fmt, ...) (void)fmt; TOUCH(__VA_ARGS__)
#endif

void log_error(const char* fmt, ...);

[[noreturn]] void fatal(const char* fmt, ...);
