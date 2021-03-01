#pragma once

// Simplistic logging for use before logger is initialized.
#define XYOutput xynq::Output
#define XYOutputError xynq::OutputError


namespace xynq {
// Writes to stdout.
void Output(const char *format, ...);

// Writes to stderr.
void OutputError(const char *format, ...);
}
