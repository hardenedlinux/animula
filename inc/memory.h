#ifndef __ANIMULA_MEMORY_H__
#define __ANIMULA_MEMORY_H__
/*  Copyright (C) 2020
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

#include "debug.h"
#include "os.h"
#include "types.h"

extern u8_t *__static_stack;
extern u8_t *__global_array;

void init_ram_heap (void);
void *os_malloc (size_t size);
void *os_calloc (size_t n, size_t size);
void os_free (void *ptr);
#endif // End of __ANIMULA_MEMORY_H__
