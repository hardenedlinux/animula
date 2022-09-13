#ifndef __ANIMULA_DEBUG_H__
#define __ANIMULA_DEBUG_H__
/*  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.

 *  Animula is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "os.h"
#include "types.h"

extern GLOBAL_DEF (bool, vm_verbose);

#define PANIC(format, ...)                                  \
  do                                                        \
    {                                                       \
      os_printk ("%s:%d %s: " format, __FILE__, __LINE__,   \
                 __FUNCTION__ __VA_OPT__ (, ) __VA_ARGS__); \
      panic ("");                                           \
    }                                                       \
  while (0)

static inline void panic (const char *reason)
{
  os_printk ("PANIC!\n");
  os_printk ("%s", reason);
#ifdef ANIMULA_LINUX
  exit (-1);
#else
  while (1)
    ;
#endif
}

#if defined ANIMULA_DEBUG
#  ifndef VM_DEBUG
#    define VM_DEBUG(...) GLOBAL_REF (vm_verbose) ? os_printk (__VA_ARGS__) : 0;
#  endif
#  ifndef assert
#    define __assert(e, file, line)                                       \
      ((void)os_printk ("%s:%u: failed assertion `%s'\n", file, line, e), \
       panic ("panic!\n"))
#    define assert(e) ((void)((e) ? 0 : __assert (#    e, __FILE__, __LINE__)))
#  endif
#else
#  define VM_DEBUG
#  ifndef assert
#    define assert
#  endif
#endif

#endif // End of __ANIMULA_DEBUG_H__
