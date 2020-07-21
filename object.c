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
const u8_t true_const = 0;
const u8_t false_const = 1;
const u8_t null_const = 2;
const u8_t none_const = 3;

void init_predefined_objects (void) {}

// This should only be called by GC
void free_object (object_t obj)
{
  gc_free (obj->value);
  /* We should set value to NULL here, since obj is not guarrenteed to be
   * freed by GC, since it could be recycled by the pool.
   */
  obj->value = NULL;
  gc_free ((void *)obj);
}
