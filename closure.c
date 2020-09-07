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

#include "closure.h"

static RB_HEAD (ClosureCacheLookup, ClosureCacheNode)
  ClosureCacheLookupHead = RB_INITIALIZER (&ClosureCacheLookupHead);

RB_GENERATE_STATIC (ClosureCacheLookup, ClosureCacheNode, entry,
                    closure_cache_compare);

static inline closure_t remove_closure_cache_node (ClosureCacheNode *cn)
{
  RB_REMOVE (ClosureCacheLookup, &ClosureCacheLookupHead, cn);
  closure_t closure = cn->closure;
  os_free (cn);
  return closure;
}

static void add_to_closure_cache (closure_t closure)
{
  ClosureCacheNode *cn
    = (ClosureCacheNode *)os_malloc (sizeof (ClosureCacheNode));
  cn->closure = closure;
  RB_INSERT (ClosureCacheLookup, &ClosureCacheLookupHead, cn);
}

static closure_t
find_from_closure_cache (reg_t entry, closure_t (*proc) (ClosureCacheNode *cn))
{
  for (ClosureCacheNode *cn
       = RB_MIN (ClosureCacheLookup, &ClosureCacheLookupHead);
       cn != NULL;
       cn = RB_NEXT (ClosureCacheLookup, &ClosureCacheLookupHead, cn))
    {
      if (entry == cn->closure->entry)
        return proc (cn);
    }

  return NULL;
}

closure_t closure_cache_fetch (reg_t entry)
{
  return find_from_closure_cache (entry, return_closure);
}

// We export this function for GC
closure_t remove_closure_cache (closure_t closure)
{
  return find_from_closure_cache (closure->entry, remove_closure_cache_node);
}

// -------- Continuation
/* object_t make_continuation () */
/* { */
/*   object_t obj = (object_t)gc_malloc (sizeof (struct Object)); */
/*   cont_t cont = (cont_t)gc_malloc (sizeof (union Continuation)); */

/*   obj->attr.gc = 0; */
/*   obj->attr.type = (otype_t)continuation; */
/*   obj->value = (void *)cont; */

/*   return obj; */
/* } */

// ----------- Closure
closure_t make_closure (u8_t arity, u8_t frame_size, reg_t entry)
{
  printf ("create new closure!\n");
  closure_t closure = closure_cache_fetch (entry);

  if (!closure)
    {
      closure = (closure_t)gc_pool_malloc (gc_closure);

      if (!closure)
        {
          closure = (closure_t)gc_malloc (sizeof (Closure)
                                          + sizeof (Object) * frame_size);
          gc_book (gc_closure, (void *)closure);
        }
    }

  closure->frame_size = frame_size;
  closure->entry = entry;
  closure->arity = arity;

  add_to_closure_cache (closure);
  return closure;
}
