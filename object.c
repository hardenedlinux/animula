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

obj_list_t new_obj_list ()
{
  return (obj_list_t)gc_malloc (sizeof (ObjectList));
}

list_t new_list ()
{
  NEW_OBJ (list_t, gc_list, List);
}

vector_t new_vector ()
{
  NEW_OBJ (vector_t, gc_vector, Vector);
}

pair_t new_pair ()
{
  NEW_OBJ (pair_t, gc_pair, Pair);
}

/* string_t new_string () */
/* { */

/* } */

closure_t new_closure ()
{
  NEW_OBJ (closure_t, gc_closure, Closure);
}

object_t new_object (u8_t type)
{
  object_t object = (object_t)gc_pool_malloc (gc_object);

  if (!object)
    {
      object = (object_t)gc_malloc (sizeof (Object));
    }

  switch (type)
    {
    case list:
      {
        object->value = (void *)new_list ();
        break;
      }
    case pair:
      {
        object->value = (void *)new_pair ();
        break;
      }
    case closure_on_heap:
      {
        object->value = (void *)new_closure ();
        break;
      }
    case vector:
      {
        object->value = (void *)new_vector ();
        break;
      }
      /* case string: */
      /*   { */
      /*     object->value = (void *)new_string (); */
      /*     break; */
      /*   } */
    }
  object->attr.type = type;
  object->attr.gc = 1;
  gc_book (gc_object, (void *)object);
  return object;
}
