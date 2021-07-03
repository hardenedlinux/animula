/*  Copyright (C) 2020-2021
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

#include "object.h"

/* Special Const
 * is used for representing these values:
 * 1. false: #f in Scheme, False in Python, false in Lua
 * 2. true: #t in Scheme, True in Python, true in Lua
 * 3. empty list: () in Scheme, [] in Python
 * 4. none: None in Python, nil in Lua
 *
 * NOTE:
 * We need to check them with pointer by the unified interface,
 * so don't use enum.
 */
GLOBAL_DEF (const Object, true_const)
  = {.attr = {.type = boolean, .gc = 0}, .value = (void *)1};
GLOBAL_DEF (const Object, false_const)
  = {.attr = {.type = boolean, .gc = 0}, .value = NULL};
GLOBAL_DEF (const Object, null_const)
  = {.attr = {.type = null_obj, .gc = 0}, .value = NULL};
GLOBAL_DEF (const Object, none_const)
  = {.attr = {.type = none, .gc = 0}, .value = NULL};

// ----------- Closure
closure_t make_closure (u8_t arity, u8_t frame_size, reg_t entry)
{
  VM_DEBUG ("create new closure!\n");

  closure_t closure
    = (closure_t)os_malloc (sizeof (Closure) + sizeof (Object) * frame_size);

  if (!closure)
    {
      return NULL;
    }

  closure->frame_size = frame_size;
  closure->entry = entry;
  closure->arity = arity;
  closure->attr.gc = 1;
  closure->attr.type = closure_on_heap;

  return closure;
}

obj_list_t lambdachip_new_obj_list_node (void)
{
  CREATE_NEW_OBJ (obj_list_t, obj_list_node, ObjectList);
}

list_t lambdachip_new_list (void)
{
  CREATE_NEW_OBJ (list_t, list, List);
}

vector_t lambdachip_new_vector (void)
{
  CREATE_NEW_OBJ (vector_t, vector, Vector);
}

pair_t lambdachip_new_pair (void)
{
  CREATE_NEW_OBJ (pair_t, pair, Pair);
}

closure_t lambdachip_new_closure (void)
{
  CREATE_NEW_OBJ (closure_t, closure_on_heap, Closure);
}

object_t lambdachip_new_object (u8_t type)
{
  bool from_pool = true;
  bool has_inner_obj = true;
  bool new_alloc = false;
  object_t object = NULL;
  void *value = NULL;

  object = (object_t)gc_pool_malloc (0);

  if (!object)
    {
      object = (object_t)os_malloc (sizeof (Object));
      new_alloc = true;

      // Alloc failed, return NULL to trigger GC
      if (!object)
        return NULL;
    }

  switch (type)
    {
    case list:
      {
        value = (void *)lambdachip_new_list ();
        break;
      }
    case pair:
      {
        value = (void *)lambdachip_new_pair ();
        break;
      }
    case closure_on_heap:
      {
        value = (void *)lambdachip_new_closure ();
        break;
      }
    case vector:
      {
        value = (void *)lambdachip_new_vector ();
        break;
      }
    default:
      {
        has_inner_obj = false;
        break;
      }
    }

  if (has_inner_obj && !value)
    {
      // The inner object wasn't successfully allocated, return NULL for GC
      free_object (object);
      return NULL;
    }

  object->value = value;
  object->attr.type = type;
  object->attr.gc = 1;

  if (!new_alloc)
    gc_book (type, object);

  return object;
}
