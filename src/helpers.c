#pragma once

#include <stddef.h>
#include <stdint.h>

/* Convert integer to string (itoa) for bare metal.
 *
 * - buf: output buffer
 * - bufsize: size of buffer in bytes (including terminating NUL)
 * - value: signed integer to convert
 * - base: 2..36
 * - returns pointer to buf on success, NULL on error (invalid base or buffer
 * too small)
 *
 * Notes:
 * - uses only simple operations, no libc.
 * - negative numbers handled only for base 10.
 * - letters are lowercase for bases > 10.
 */

char *itoa_bare(char *buf, size_t bufsize, long value, int base) {
  if (!buf || bufsize == 0)
    return NULL;
  if (base < 2 || base > 36)
    return NULL;

  char *p = buf;
  size_t i = 0;

  // handle zero specially
  if (value == 0) {
    if (bufsize < 2)
      return NULL; // need at least "0\0"
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  int negative = 0;
  unsigned long uvalue;

  if (value < 0 && base == 10) {
    negative = 1;
    // prevent overflow for LONG_MIN
    uvalue = (unsigned long)(-(value + 1)) + 1;
  } else {
    uvalue = (unsigned long)value;
  }

  // convert digits into temporary buffer in reverse order
  // worst-case digits count for base 2 of 64-bit is 64, add 1 for sign and 1
  // for NUL we will write directly into user buffer from the end to avoid extra
  // alloc, but simpler to use a small local temp array sized to the max needed.
  char tmp[66]; // safe for 64-bit binary plus sign and NUL
  size_t ti = 0;

  while (uvalue && ti < sizeof(tmp)) {
    unsigned long rem = uvalue % (unsigned long)base;
    if (rem < 10)
      tmp[ti++] = (char)('0' + rem);
    else
      tmp[ti++] = (char)('a' + (rem - 10)); // lowercase letters
    uvalue = uvalue / (unsigned long)base;
  }

  if (negative) {
    tmp[ti++] = '-';
  }

  // now tmp[0..ti-1] holds reversed characters
  // check buffer size: need ti chars + 1 for NUL
  if (ti + 1 > bufsize)
    return NULL;

  // reverse into output buffer
  for (size_t j = 0; j < ti; j++) {
    buf[j] = tmp[ti - 1 - j];
  }
  buf[ti] = '\0';
  return buf;
}

/* Convenience wrapper for unsigned values (works with any base).
 * Same semantics: returns buf or NULL.
 */
char *utoa_bare(char *buf, size_t bufsize, unsigned long value, int base) {
  if (!buf || bufsize == 0)
    return NULL;
  if (base < 2 || base > 36)
    return NULL;

  if (value == 0) {
    if (bufsize < 2)
      return NULL;
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  char tmp[66];
  size_t ti = 0;
  unsigned long u = value;

  while (u && ti < sizeof(tmp)) {
    unsigned long rem = u % (unsigned long)base;
    if (rem < 10)
      tmp[ti++] = (char)('0' + rem);
    else
      tmp[ti++] = (char)('a' + (rem - 10));
    u /= (unsigned long)base;
  }

  if (ti + 1 > bufsize)
    return NULL;

  for (size_t j = 0; j < ti; j++)
    buf[j] = tmp[ti - 1 - j];
  buf[ti] = '\0';
  return buf;
}

uint16_t swap_endian16(uint16_t val) { return (val << 8) | (val >> 8); }

// uint8_t *wtob(uint16_t *val, size_t valueCount) {
//   uint16_t ret[valueCount];
//   for (size_t i = 0; i < valueCount; i++)
//     ret[i] = (val[i] << 8) | (val[i] >> 8);
//   return (uint8_t *)ret;
// }
