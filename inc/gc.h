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

typedef enum gc_obj_type
{
  gc_object,
  gc_pair,
  gc_vector,
  gc_continuation,
  gc_list,
  gc_closure,
  gc_obj_list
} gobj_t;

#define MALLOC_FROM_POOL(lst)               \
  do                                        \
    {                                       \
      obj_list_t node = SLIST_FIRST (&lst); \
      if (node)                             \
        {                                   \
          SLIST_REMOVE_HEAD (&lst, next);   \
          ret = node->obj;                  \
        }                                   \
    }                                       \
  while (0)

void gc_init (void);
bool gc (void);
void *gc_malloc (size_t size);
void gc_free (void *ptr);
void *gc_pool_malloc (gobj_t type);
void gc_book (gobj_t type, void *obj);

#endif // End of __LAMBDACHIP_GC_H__
