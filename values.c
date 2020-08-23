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

#include "values.h"

// -------- Continuation
object_t make_continuation ()
{
  object_t obj = (object_t)gc_malloc (sizeof (struct Object));
  cont_t cont = (cont_t)gc_malloc (sizeof (union Continuation));

  obj->attr.gc = 0;
  obj->attr.type = (otype_t)continuation;
  obj->value = (void *)cont;

  return obj;
}

// ----------- Closure
closure_t make_closure (u8_t arity, u8_t frame_size, reg_t entry)
{
  closure_t closure
    = (closure_t)gc_malloc (sizeof (Closure) + sizeof (Object) * frame_size);

  closure->frame_size = frame_size;
  closure->entry = entry;
  closure->arity = arity;
  return closure;
}
