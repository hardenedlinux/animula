#ifndef __LAMBDACHIP_GC_H__
#define __LAMBDACHIP_GC_H__
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

#include "memory.h"
#include "object.h"
#include "types.h"

void gc_init (void);
void gc_book (object_t obj);
bool gc (void);
void *gc_malloc (size_t size);
void gc_free (void *ptr);

#endif // End of __LAMBDACHIP_GC_H__
