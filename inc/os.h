#ifndef __LAMBDACHIP_OS_H__
#define __LAMBDACHIP_OS_H__
/*  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *  Lambdachip is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.

 *  Lambdachip is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LAMBDACHIP_GC_HURT
#  define LAMBDACHIP_GC_HURT 0
#endif

#if defined LAMBDACHIP_ZEPHYR
#  include <sys/printk.h>
#  include <zephyr.h>
#  define os_printk           printk
#  define os_snprintk         snprintk
#  define get_platform_info() CONFIG_BOARD
#  include <kernel.h>
#  define __malloc malloc
#  define __calloc calloc
#  define __free   free
#  include <string.h>
#  define os_memset memset
#  define os_memcpy memcpy
#  define os_strlen strlen
#  include <math.h>
#  define os_abs        abs
#  define os_fabs       fabs
#  define STATIC_ASSERT BUILD_ASSERT
/* NOTE: The newlib in Zephyr doesn't support strnlen, unfortunately. */
static inline size_t os_strnlen (const char *s, size_t n)
{
  size_t len = os_strlen (s);
  if (len > n)
    return n;
  else
    return len;
}
#  define os_strncmp strncmp
#  define os_usleep  k_usleep
#  include <console/console.h>
#  define os_getchar console_getchar
#  define os_getline console_getline
#  include <device.h>
#  include <drivers/flash.h>
#  include <fs/fs.h>
#  include <kernel.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <sys/cdefs.h>
#  define os_snprintf snprintf
int zephyr_open (const char *pathname, int flags);
int zephyr_close (int fd);
ssize_t zephyr_read (int fd, void *buf, size_t count);
int zephyr_stat (const char *path, struct fs_dirent *entry);

#elif defined LAMBDACHIP_LINUX
#  include <assert.h>
#  include <stdio.h>
#  include <sys/cdefs.h>
#  include <unistd.h>
#  define os_snprintf         snprintf
#  define os_printk           printf
#  define os_getchar          getchar
#  define os_getline          getline
#  define get_platform_info() "GNU/Linux"
#  include <stdlib.h>
#  define __malloc malloc
#  define __calloc calloc
#  define __free   free
#  include <string.h>
#  define os_memset  memset
#  define os_memcpy  memcpy
#  define os_strnlen strnlen
#  define os_strncmp strncmp
#  define os_usleep  usleep
#  include <math.h>
#  define os_abs     abs
#  define os_fabs    fabs
#  define os_getchar getchar
#  if defined __x86_64__
#    define ADDRESS_64
#  endif
#  define linux_open       open
#  define linux_read       read
#  define linux_close      close
#  define linux_stat       stat
#  define STATIC_ASSERT(e) _Static_assert(e, "assert failed: " #  e)
#else
#  error "Please specify a platform!"
#endif

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#  define LAMBDACHIP_LITTLE_ENDIAN
#else
#  define LAMBDACHIP_BIG_ENDIAN
#endif

/* BITS_LITTLE: Lowest addressed means least significant.
 * BITS_BIG: Lowest addressed means most significant.
 */
#if defined LAMBDACHIP_ZEPHYR
/* zephyr on stm32 F4, the bit-fields are big endian
 */
#  define LAMBDACHIP_BITS_BIG
#elif defined LAMBDACHIP_LINUX
/* According to Linux i386 ABI, bit-fields are big endian
 * https://refspecs.linuxfoundation.org/elf/abi386-4.pdf
 */
#  define LAMBDACHIP_BITS_BIG
#endif

// __LLP64__ is for Windows, even it's GCC
#if defined(__LP64__) || defined(__LLP64__)
#  define ADDRESS_64
#endif

#define GLOBAL_REF(k) ____lambdachip_global_var_##k

#define GLOBAL_DEF(t, k) t GLOBAL_REF (k)

// TODO: make it atomic
#define GLOBAL_SET(k, v)    \
  do                        \
    {                       \
      GLOBAL_REF (k) = (v); \
    }                       \
  while (0)

#ifdef FORCE_MEMORY_SIZE_TEST
#  define MEMORY_TRACKER_SIZE 4000
#endif

#ifndef PRE_ARN
#  define PRE_ARN 100
#endif

#ifndef PRE_OLN
#  define PRE_OLN 100
#endif

#endif // End of __LAMBDACHIP_OS_H__
