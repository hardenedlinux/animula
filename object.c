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

void init_predefined_objects (void) {}

// This should only be called by GC
void free_object (object_t obj)
{
  switch (obj->attr.type)
    {
    case imm_int:
    case pair:
    case list:
    case symbol:
    case continuation:
      {
        gc_free (obj->value);
        obj->value = NULL;
        break;
      }
    default:
      break;
    }
  /* We should set value to NULL here, since obj is not guarrenteed to be
   * freed by GC, and it could be recycled by the pool.
   */
  gc_free (obj);
  obj = NULL;
}

obj_list_t new_obj_list ()
{
  obj_list_t ol = (obj_list_t)gc_pool_malloc (gc_obj_list);

  if (!ol)
    {
      ol = (obj_list_t)gc_malloc (sizeof (ObjectList));
      gc_book (gc_obj_list, (void *)ol);
    }

  return ol;
}

list_t new_list ()
{
  list_t ll = (list_t)gc_pool_malloc (gc_list);

  if (!ll)
    {
      ll = (list_t)gc_malloc (sizeof (List));
      gc_book (gc_list, (void *)ll);
    }

  return ll;
}

vector_t new_vector ()
{
  vector_t v = (vector_t)gc_pool_malloc (gc_vector);

  if (!v)
    {
      v = (vector_t)gc_malloc (sizeof (Vector));
      gc_book (gc_vector, (void *)v);
    }

  return v;
}

object_t new_object (u8_t type)
{
  object_t object = (object_t)gc_malloc (sizeof (Object));
  object->attr.type = type;
  return object;
}
