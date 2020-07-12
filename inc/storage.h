#ifndef __LAMBDACHIP_STORAGE_H__
#define __LAMBDACHIP_STORAGE_H__
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
#include "debug.h"
#include "stdio.h"
#include "vm.h"

#if defined LAMBDACHIP_ZEPHYR
// lds magic
extern char _flash_used[];
#define LAMBDACHIP_FLASH_AVAILABLE_OFFSET (u32_t)_flash_used
#else
#define LAMBDACHIP_FLASH_AVAILABLE_OFFSET 0
#endif

#define LAMBDACHIP_FLASH_TMP_BUF_SIZE 256

static inline u32_t uart_get_u32(void)
{
  u8_t ret[4] = {0};

#if defined LAMBDACHIP_BIG_ENDIAN
  ret[0] = getchar();
  ret[1] = getchar();
  ret[2] = getchar();
  ret[3] = getchar();
#else
  ret[3] = getchar();
  ret[2] = getchar();
  ret[1] = getchar();
  ret[0] = getchar();
#endif
  return *((u32_t*)ret);
}

static inline u16_t uart_get_u16(void)
{
  u8_t ret[2] = {0};

#if defined LAMBDACHIP_BIG_ENDIAN
  ret[0] = getchar();
  ret[1] = getchar();
#else
  ret[1] = getchar();
  ret[0] = getchar();
#endif
  return *((u16_t*)&ret);
}

static inline u8_t uart_get_u8(void)
{
  return getchar();
}

static inline void uart_drop_rest_data(void)
{
  while(!getchar());
}

void os_flash_init(void);
int os_flash_erase(size_t offset, size_t size);
int os_flash_write(const char* buf, size_t offset, size_t size);
int os_flash_read(char* buf, size_t offset, size_t size);
#endif // End of __LAMBDACHIP_STORAGE_H__
