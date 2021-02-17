#ifndef __LAMBDACHIP_DEBUG_H__
#define __LAMBDACHIP_DEBUG_H__
/*  Copyright (C) 2020
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

#include "os.h"
#include "types.h"

extern GLOBAL_DEF (bool, vm_verbose);

static inline void panic (const char *reason)
{
  os_printk ("%s", reason);
  while (1)
    ;
}

#if defined LAMBDACHIP_DEBUG
#  ifndef VM_DEBUG
#    define VM_DEBUG(...) GLOBAL_REF (vm_verbose) ? os_printk (__VA_ARGS__) : 0;
#  endif
#  ifndef LAMBDACHIP_LINUX
#    define __assert(e, file, line)                                       \
      ((void)os_printk ("%s:%u: failed assertion `%s'\n", file, line, e), \
       panic ("panic!\n"))
#    define assert(e) ((void)((e) ? 0 : __assert (#    e, __FILE__, __LINE__)))
#  endif
#else
#  define VM_DEBUG
#  define assert
#endif

#endif // End of __LAMBDACHIP_DEBUG_H__
