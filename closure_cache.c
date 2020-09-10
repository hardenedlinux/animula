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

#include "closure_cache.h"

static RB_HEAD (ClosureCacheLookup, ClosureCacheNode)
  ClosureCacheLookupHead = RB_INITIALIZER (&ClosureCacheLookupHead);

RB_GENERATE_STATIC (ClosureCacheLookup, ClosureCacheNode, entry,
                    closure_cache_compare);

static inline closure_t remove_closure_cache_node (ClosureCacheNode *cn)
{
  RB_REMOVE (ClosureCacheLookup, &ClosureCacheLookupHead, cn);
  closure_t closure = cn->closure;
  os_free (cn);
  /* NOTE:
   * Don't remove closure here, it's GC's work.
   */
  return closure;
}

void add_to_closure_cache (closure_t closure)
{
  ClosureCacheNode *cn
    = (ClosureCacheNode *)os_malloc (sizeof (ClosureCacheNode));
  cn->closure = closure;
  RB_INSERT (ClosureCacheLookup, &ClosureCacheLookupHead, cn);
}

closure_t find_from_closure_cache (reg_t entry,
                                   closure_t (*proc) (ClosureCacheNode *cn))
{
  /* for (ClosureCacheNode *cn */
  /*      = RB_MIN (ClosureCacheLookup, &ClosureCacheLookupHead); */
  /*      cn != NULL; */
  /*      cn = RB_NEXT (ClosureCacheLookup, &ClosureCacheLookupHead, cn)) */
  /*   { */
  /*     if (entry == cn->closure->entry) */
  /*       return proc (cn); */
  /*   } */

  /* return NULL; */
  Closure c = {.entry = entry};
  ClosureCacheNode node = {.closure = &c};
  ClosureCacheNode *cn = NULL;
  cn = RB_FIND (ClosureCacheLookup, &ClosureCacheLookupHead, &node);
  return cn ? proc (cn) : NULL;
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
