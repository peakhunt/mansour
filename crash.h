#pragma once

// Macro to automatically capture file and line information
#define CRASH(fmt, ...) \
    generic_crash(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

extern void __attribute__((noreturn)) generic_crash(const char *file, int line, const char *fmt, ...);
