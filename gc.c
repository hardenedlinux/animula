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

#include "gc.h"

static obj_list_head_t gc_free_list;

static void mark(object_t obj)
{
  // TODO
}

static void sweep(void)
{
  // TODO
}

void gc_init(void)
{
  SLIST_INIT(&gc_free_list);
}

bool gc(void)
{
  /* TODO:
   * 1. Obj pool is empty, goto 3
   * 2. Free all unused obj:
   *    a. move from ref_list to free_list (obj pool)
   *    b. if no collectable obj, then goto 3
   * 3. Free obj pool
   */

  return false;
}

static inline void* pool_malloc(size_t size)
{
  /* NOTE:
   * Object pool design is based on the facts:
   *    1. VM only allocates objects with gc_malloc
   *    2. All objects are well defined and fixed sized
   *    3. All objects are recycleable in runtime
   * That's why pool_malloc is useful here.
   */

  /* TODO:
   * 1. Find a proper sized object in free_list
   * 2. If succeed, move it to ref_list
   */

  return NULL;
}

void* gc_malloc(size_t size)
{
  do
    {
      /* NOTE:
       * 1. gc_malloc will allocate from free_list (obj pool) first
       * 2. Call malloc if a or b meets:
       *    a. obj pool is empty
       *    b. obj pool has no suitable obj
       * 3. malloc is failed:
       *    a. collect the whole obj pool
       *    b. malloc again
       *    c. malloc is still failed, then waiting and printing error
       */
      void* ptr = pool_malloc(size);
      if(ptr) return ptr;

      ptr = os_malloc(size);
      if(ptr) return ptr;

      gc();
    } while(1);
}

// gc_free shouldn't be called explicitly, it should be only used by GC
void gc_free(void* ptr)
{
  // TODO: recycle to pool, if pool size exceeds to max, then call low-level free.
}
