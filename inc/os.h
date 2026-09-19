#ifndef __ANIMULA_OS_H__
#define __ANIMULA_OS_H__
/*  Copyright (C) 2026 HardenedLinux Community
 *        Nala Ginrut <roy@hardenedlinux.org>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.
 *
 *  Animula is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  os.h is the OS-abstraction entry point for the VM and Primitive layers.
 *
 *  Platform capabilities reach this file through inc/vos.h, which selects
 *  one per-platform OAL header (inc/vos/oal/linux/vos.h or
 *  inc/vos/oal/zephyr/vos.h) that maps the os_* names onto that platform's
 *  real APIs.  Everything in this file beyond that include is VM build
 *  configuration (compile-time assertion idiom, endianness, global-variable
 *  macros, memory-tracker sizing), not an OS runtime capability.
 */

#include "vos.h"

/* --- compile-time assertion idiom (platform specific) --- */
#if defined ANIMULA_ZEPHYR
#  define STATIC_ASSERT BUILD_ASSERT
#elif defined ANIMULA_LINUX
#  define STATIC_ASSERT(e) _Static_assert(e, "assert failed: " #  e)
#endif

/* --- endianness (VM build config) --- */
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#  define ANIMULA_LITTLE_ENDIAN
#else
#  define ANIMULA_BIG_ENDIAN
#endif

/* BITS_LITTLE: Lowest addressed means least significant.
 * BITS_BIG: Lowest addressed means most significant.
 */
#if defined ANIMULA_ZEPHYR
/* zephyr on stm32 F4, the bit-fields are big endian
 */
#  define ANIMULA_BITS_BIG
#elif defined ANIMULA_LINUX
/* According to Linux i386 ABI, bit-fields are big endian
 * https://refspecs.linuxfoundation.org/elf/abi386-4.pdf
 */
#  define ANIMULA_BITS_BIG
#  if defined __x86_64__
#    define ADDRESS_64
#  endif
#endif

// __LLP64__ is for Windows, even it's GCC
#if defined(__LP64__) || defined(__LLP64__)
#  define ADDRESS_64
#endif

#define GLOBAL_REF(k) ____animula_global_var_##k

#define GLOBAL_DEF(t, k) t GLOBAL_REF (k)

// TODO: make it atomic
#define GLOBAL_SET(k, v)    \
  do                        \
    {                       \
      GLOBAL_REF (k) = (v); \
    }                       \
  while (0)

#ifdef FORCE_MEMORY_SIZE_TEST
#  define MEMORY_TRACKER_ARRAY_LENGTH 4000
#  define MEMORY_HARD_LIMIT           12000
#endif

#ifndef PRE_ARN
#  define PRE_ARN 100
#endif

#ifndef PRE_OLN
#  define PRE_OLN 100
#endif

#endif // End of __ANIMULA_OS_H__
