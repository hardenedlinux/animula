#ifndef __LAMBDACHIP_MEMORY_H__
#define __LAMBDACHIP_MEMORY_H__
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
#include "debug.h"

#define os_memset __memset
#define os_memcpy __memcpy

#define SS_MAX_SIZE 100
#define GARR_MAX_SIZE 50

extern u8_t* __static_stack;
extern u8_t* __global_array;
extern u32_t __store_offset;

/* FIXME: Don't check bound here, since it's slower.
 *        It's possible to verify it in LEF with a tool.
 */

static inline u8_t ss_read_u8(u8_t offset)
{
  if(offset >= SS_MAX_SIZE)
    {
      /* FIXME:
       * Shouldn't panic here, the elegant way is to return to the ready
       * state of VM, and stop running the current LEF.
       */
      os_printk("ss_read_u8: Invalid offset %u\n", offset);
      panic("Fatal error when read from static stack!\n");
    }
  return __static_stack[offset];
}

static inline u16_t ss_read_u16(u8_t offset)
{
  if(offset >= SS_MAX_SIZE)
    {
      /* FIXME:
       * Shouldn't panic here, the elegant way is to return to the ready
       * state of VM, and stop running the current LEF.
       */
      os_printk("ss_read_u16: Invalid offset %u\n", offset);
      panic("Fatal error when read from static stack!\n");
    }
  return ((u16_t*)__static_stack)[offset];
}

static inline u32_t ss_read_u32(u8_t offset)
{
  if(offset >= SS_MAX_SIZE)
    {
      /* FIXME:
       * Shouldn't panic here, the elegant way is to return to the ready
       * state of VM, and stop running the current LEF.
       */
      os_printk("ss_read_u32: Invalid offset %u\n", offset);
      panic("Fatal error when read from static stack!\n");
    }
  return ((u32_t*)__static_stack)[offset];
}

/* NOTE:
 * return u16_t as the ss offset.
 */
static inline u16_t ss_store_u32(u32_t addr)
{
  u32_t *ptr = (u32_t*)(__static_stack + __store_offset);
  *ptr = addr;
  __store_offset += 4;

  return __store_offset - 4;
}

static inline u8_t ss_store_tiny_encode(u32_t addr)
{
  __static_stack[++__store_offset] = addr;
  return __store_offset;
}

static inline u8_t global_get(u8_t offset)
{
  if(offset >=  GARR_MAX_SIZE)
    {
      /* FIXME:
       * Shouldn't panic here, the elegant way is to return to the ready
       * state of VM, and stop running the current LEF.
       */
      os_printk("ss_read_u8: Invalid offset %u\n", offset);
      panic("Fatal error when read from static stack!\n");
    }
  return __global_array[offset];
}

static inline void global_set(u8_t offset, u8_t data)
{
  if(offset >=  GARR_MAX_SIZE)
    {
      /* FIXME:
       * Shouldn't panic here, the elegant way is to return to the ready
       * state of VM, and stop running the current LEF.
       */
      os_printk("ss_read_u8: Invalid offset %u\n", offset);
      panic("Fatal error when read from static stack!\n");
    }
  __global_array[offset] = data;
}

void init_ram_heap(void);
void* os_malloc(size_t size);
void os_free(void* ptr);
#endif // End of __LAMBDACHIP_MEMORY_H__
