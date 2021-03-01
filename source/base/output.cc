#include "output.h"

#include <cstdarg>
#include <cstdio>

void xynq::Output(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(::stdout, format, args);
    fputs("\n", ::stdout);
    va_end (args);
}

void xynq::OutputError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(::stderr, format, args);
    fputs("\n", ::stderr);
    va_end (args);
}