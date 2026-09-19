#ifndef __ANIMULA_VOS_OAL_ZEPHYR_H__
#define __ANIMULA_VOS_OAL_ZEPHYR_H__

/*
 *  Copyright (C) 2026 HardenedLinux Community
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
 *  Zephyr OAL: maps the OS abstraction (os_*) onto the ZephyrRTOS / newlib
 *  capabilities of a Cortex-M target (e.g. the "Alonzo" board).
 *
 *  This file is selected by vos.h when ANIMULA_ZEPHYR is defined.  The VM
 *  and Primitive layers never include Zephyr headers directly -- they call
 *  the os_* names defined here.  Zephyr API names are never the OS
 *  abstraction themselves; this file is the mapping.
 */

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/fs/fs.h>

/* --- console --- */
#define os_printk(...)        printk (__VA_ARGS__)
#define os_getchar()          console_getchar ()
#define os_getline(...)       console_getline (__VA_ARGS__)

/* --- formatting --- */
#define os_snprintf(...)      snprintf (__VA_ARGS__)

/* --- memory / basic string library --- */
#define os_memcpy(d, s, n)    memcpy ((d), (s), (n))
#define os_memset(s, c, n)    memset ((s), (c), (n))
#define os_strncmp(a, b, n)   strncmp ((a), (b), (n))
#define os_strchr(s, c)       strchr ((s), (c))
#define os_strncpy(d, s, n)   strncpy ((d), (s), (n))
#define os_strlen(s)          strlen ((s))
#define os_abs(x)             abs ((x))
#define os_fabs(x)            fabs ((x))

/* NOTE: the newlib bundled with Zephyr does not provide strnlen, so the
 * "bounded string length" capability is backed by a small shim here. */
static inline size_t
os_strnlen (const char *s, size_t n)
{
  size_t len = os_strlen (s);
  if (len > n)
    return n;
  return len;
}

/* --- raw allocator (backs the VM-level os_malloc/os_calloc/os_free) --- */
#define __malloc(sz)          malloc (sz)
#define __calloc(n, sz)       calloc ((n), (sz))
#define __free(p)             free (p)

/* --- time: monotonic elapsed ticks (cycle count) --- */
static inline uint64_t
os_timestamp (void)
{
  return (uint64_t)k_cycle_get_32 ();
}

/* --- sleep --- */
#define os_usleep(us)         k_usleep (us)

/* --- platform --- */
#define get_platform_info()   CONFIG_BOARD

/* --- termination --- */
#define os_abort(code)        do { (void)(code); while (1) ; } while (0)

/* --- file I/O (implemented in storage.c / os.c) --- */
/* Only read-only open is exercised today; add a write/rdwr flag when a
 * caller actually needs one (the OS abstraction grows from delivery
 * history, not ahead). */
#define OS_O_RDONLY 0x01

int os_open (const char *path, int flags);
int os_close (int fd);
int os_file_exist (const char *path);

/* Zephyr filesystem backend (implemented in os.c) */
int zephyr_open (const char *pathname, int flags);
int zephyr_close (int fd);
ssize_t zephyr_read (int fd, void *buf, size_t count);
int zephyr_stat (const char *path, struct fs_dirent *entry);

#endif /* __ANIMULA_VOS_OAL_ZEPHYR_H__ */
