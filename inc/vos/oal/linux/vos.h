#ifndef __ANIMULA_VOS_OAL_LINUX_H__
#define __ANIMULA_VOS_OAL_LINUX_H__

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
 *  Linux OAL: maps the OS abstraction (os_*) onto the POSIX/libc
 *  capabilities of a GNU/Linux host.
 *
 *  This file is selected by vos.h when ANIMULA_LINUX is defined.  The VM
 *  and Primitive layers never include platform headers directly -- they
 *  call the os_* names defined here.  Linux is only *one* OAL target, not
 *  the definition of the OS abstraction itself.
 */

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* --- console --- */
#define os_printk(...)        printf (__VA_ARGS__)
#define os_getchar()          getchar ()
#define os_getline(...)       getline (__VA_ARGS__)

/* --- formatting --- */
#define os_snprintf(...)      snprintf (__VA_ARGS__)

/* --- memory / basic string library --- */
#define os_memcpy(d, s, n)    memcpy ((d), (s), (n))
#define os_memset(s, c, n)    memset ((s), (c), (n))
#define os_strnlen(s, n)      strnlen ((s), (n))
#define os_strncmp(a, b, n)   strncmp ((a), (b), (n))
#define os_strchr(s, c)       strchr ((s), (c))
#define os_strncpy(d, s, n)   strncpy ((d), (s), (n))
#define os_strlen(s)          strlen ((s))
#define os_abs(x)             abs ((x))
#define os_fabs(x)            fabs ((x))

/* --- raw allocator (backs the VM-level os_malloc/os_calloc/os_free) --- */
#define __malloc(sz)          malloc (sz)
#define __calloc(n, sz)       calloc ((n), (sz))
#define __free(p)             free (p)

/* --- time: monotonic elapsed ticks (nanoseconds) --- */
static inline uint64_t
os_timestamp (void)
{
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* --- sleep --- */
#define os_usleep(us)         usleep (us)

/* --- platform --- */
#define get_platform_info()   "GNU/Linux"

/* --- termination --- */
#define os_abort(code)        exit (code)

/* --- file I/O (implemented in storage.c) --- */
/* Only read-only open is exercised today; add a write/rdwr flag when a
 * caller actually needs one (the OS abstraction grows from delivery
 * history, not ahead). */
#define OS_O_RDONLY 0x01

int os_open (const char *path, int flags);
int os_close (int fd);
int os_file_exist (const char *path);

#endif /* __ANIMULA_VOS_OAL_LINUX_H__ */
