#ifndef __LAMBDACHIP_VALUES_H__
#define __LAMBDACHIP_VALUES_H__
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
#include "object.h"
#include "rbtree.h"
#include "types.h"

typedef struct ClosureCacheNode ClosureCacheNode;
typedef struct ClosureCacheLookup ClosureCacheLookup;

struct ClosureCacheNode
{
  RB_ENTRY (ClosureCacheNode) entry;
  closure_t closure;
};

static inline closure_t return_closure (ClosureCacheNode *cn)
{
  return cn->closure;
}

static inline int closure_cache_compare (ClosureCacheNode *a,
                                         ClosureCacheNode *b)
{
  return (b->closure->entry - a->closure->entry);
}

/* static inline object_t gen_boolean(bool value) */
/* { */
/*   object_t obj = (object_t)gc_malloc(sizeof(struct Object)); */
/*   obj->attr.type = (otype_t)boolean; */
/*   obj->value = value ? (void*)&true_const : (void*)&false_const; */

/*   return obj; */
/* } */

closure_t remove_closure_cache (closure_t closure);
object_t make_continuation (void);
closure_t closure_cache_fetch (reg_t entry);
closure_t make_closure (u8_t arity, u8_t frame_size, reg_t entry);

#endif // End of __LAMBDACHIP_VALUES_H__;
