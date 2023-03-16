/* SPDX-License-Identifier: MIT */
#ifndef UC_IO_H
#define UC_IO_H

#include "config.h"

#ifdef UC_IO_STDOUT

/*
 * Use stdout for output.
 *
 * This works for the example program as well as several microcontroller
 * libraries with stdio support.
 */
#include <stdio.h>
#define uc_io_putc(c) fputc((c), stdout)
#define uc_io_puts(s) fputs((s), stdout)

#elif UC_IO_PUTCHAR

/*
 * Use putchar function for output.
 *
 * This uses a simple putchar function for output and implements an inline
 * loop for printing full strings.
 */
#define uc_io_putc(c) putchar(c)
#define uc_io_puts(s) do { const char *_s = (s); while (*_s) putchar(*(_s++)); }

#elif UC_IO_CUSTOM

#define uc_io_putc UC_IO_PUTC
#define uc_io_puts UC_IO_PUTS

#else

#error "Need output implementation"

#endif

#endif
