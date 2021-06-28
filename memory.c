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

#include "memory.h"

/* NOTE:
 * There're two types of stack in lambdachip:
 *   1. These are the same thing:
 *         a. dynamic stack
 *         b. runtime stack
 *         c. vm->stack
 *   2. These are another same thing:
 *         a. static stack (or static storage)
 *         b. compile time stack
 *         c. __static_stack
 *      Its contents are confirmed in the compile time, and stored in
 *      the lambdachip executable format (LEF). It'll be mapped to
 *      __static_stack when LEF is loaded by VM.
 */
u8_t *__static_stack = NULL;
u8_t *__global_array = NULL;

// malloc and free should work after init_ram_heap
void init_ram_heap (void)
{
#if defined LAMBDACHIP_ZEPHYR
  os_printk ("MM is managed by zephyr.\n");
#else
#  ifndef LAMBDACHIP_LINUX
  os_printk ("MM is in raw mode.\n");
#  endif
#endif
}

void *raw_malloc (size_t size)
{
#if defined LAMBDACHIP_BOOTSTRAP
#  error "raw_malloc hasn't been implemented yet!"
#endif
  return NULL;
}

void raw_free (void *ptr)
{
#if defined LAMBDACHIP_BOOTSTRAP
#  error "raw_free hasn't been implemented yet!"
#endif
}

void *os_malloc (size_t size)
{
  void *ptr = (void *)__malloc (size);

  if (NULL == ptr)
    {
      VM_DEBUG ("Failed to allocate memory!\n");
    }

  return ptr;
}

void *os_calloc (size_t n, size_t size)
{
  void *ptr = (void *)__calloc (n, size);

  if (NULL == ptr)
    {
      VM_DEBUG ("Failed to allocate memory!\n");
    }

  return ptr;
}

void os_free (void *ptr)
{
  if (NULL != ptr)
    __free (ptr);
}
