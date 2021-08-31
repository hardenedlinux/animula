/*  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *        Rafael Lee    <rafaellee.img@gmail.com>
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

#ifdef FORCE_MEMORY_SIZE_TEST
struct memory_block
{
  void *ptr;
  size_t size;
};

static struct memory_block g_used_memory[MEMORY_TRACKER_ARRAY_LENGTH] = {0};
#endif

void *os_malloc (size_t size)
{
#ifdef FORCE_MEMORY_SIZE_TEST
  uint32_t used_memory_size = 0;
  for (size_t i = 0; i < MEMORY_TRACKER_ARRAY_LENGTH; i++)
    {
      used_memory_size += g_used_memory[i].size;
    }

  if ((used_memory_size + size) > MEMORY_HARD_LIMIT)
    {
      VM_DEBUG ("os_malloc: Failed to allocate memory!\n");
      return NULL;
    }
#endif

  void *ptr = (void *)__malloc (size);

  if (NULL == ptr)
    {
      VM_DEBUG ("os_calloc: Failed to allocate memory!\n");
    }
#ifdef FORCE_MEMORY_SIZE_TEST
  else
    {
      size_t i = 0;
      for (; i < MEMORY_TRACKER_ARRAY_LENGTH; i++)
        {
          if (NULL == g_used_memory[i].ptr)
            {
              g_used_memory[i].ptr = ptr;
              g_used_memory[i].size = size;
              break;
            }
        }

      if (MEMORY_TRACKER_ARRAY_LENGTH == i)
        {
          PANIC ("os_malloc: memory tracker used up! %p\n", ptr);
        }
    }
#endif

  return ptr;
}

void *os_calloc (size_t n, size_t size)
{
#ifdef FORCE_MEMORY_SIZE_TEST
  uint32_t used_memory_size = 0;
  for (size_t i = 0; i < MEMORY_TRACKER_ARRAY_LENGTH; i++)
    {
      used_memory_size += g_used_memory[i].size;
    }

  if ((used_memory_size + n * size) > MEMORY_HARD_LIMIT)
    {
      VM_DEBUG ("os_calloc: Failed to allocate memory!\n");
      return NULL;
    }
#endif

  void *ptr = (void *)__calloc (n, size);

  if (NULL == ptr)
    {
      VM_DEBUG ("os_calloc: Failed to allocate memory!\n");
    }
#ifdef FORCE_MEMORY_SIZE_TEST
  else
    {
      size_t i = 0;
      for (; i < MEMORY_TRACKER_ARRAY_LENGTH; i++)
        {
          if (NULL == g_used_memory[i].ptr)
            {
              g_used_memory[i].ptr = ptr;
              g_used_memory[i].size = size;
              break;
            }
        }
      if (MEMORY_TRACKER_ARRAY_LENGTH == i)
        {
          PANIC ("os_calloc: memory tracker used up! %p\n", ptr);
        }
    }
#endif

  return ptr;
}

void os_free (void *ptr)
{
  // According to Linux library call, if ptr is NULL, no operation is performed.
  // NULL ptr checking shall not be in this level
  // Make it more strict to prevent future bug.
  // FIXME: do not panic when free null pointer
  if (NULL != ptr)
    __free (ptr);
  else
    PANIC ("Free a NULL ptr\n");

#ifdef FORCE_MEMORY_SIZE_TEST
  size_t i = 0;
  for (; i < MEMORY_TRACKER_ARRAY_LENGTH; i++)
    {
      if (ptr == g_used_memory[i].ptr)
        {
          g_used_memory[i].ptr = NULL;
          g_used_memory[i].size = 0;
          break;
        }
    }

  if (MEMORY_TRACKER_ARRAY_LENGTH == i)
    {
      PANIC ("os_free: freed a memory area not managed by us, ptr=%p\n", ptr);
    }
#endif
}
