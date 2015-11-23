/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
 *
 * Author: Simon Josefsson
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <string.h>
#include <stdarg.h>

#ifndef __attribute__
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(Spec)     /* empty */
#endif
#endif

extern int debug;
extern int error_count;
extern int break_on_error;

extern void fail (const char *format, ...)
__attribute__ ((format (printf, 1, 2)));
extern void success (const char *format, ...)
__attribute__ ((format (printf, 1, 2)));

extern void escapeprint (const char *str, size_t len);
extern void hexprint (const void *str, size_t len);
extern void binprint (const void *str, size_t len);

extern uint8_t * decode_hex_dup(const char *hex);
extern int decode_hex(uint8_t *dst, const char *h);
extern unsigned decode_hex_length(const char *h);

#define H(x)  decode_hex_dup(x)
#define HL(x) decode_hex_dup(x), decode_hex_length(x)

extern int
compare_buffer(const uint8_t *buffer, const uint32_t length_buffer,
               const uint8_t *pattern, const uint32_t length_pattern);

/* This must be implemented elsewhere. */
extern void doit (void);

#endif /* TEST_UTIL_H_ */
