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
#include "types.h"

static inline object_t gen_boolean(bool value)
{
  object_t obj = (object_t)gc_malloc(sizeof(struct Object));
  obj->attr.type = (otype_t)boolean;
  obj->value = value ? (void*)&true_const : (void*)&false_const;

  return obj;
}

object_t make_continuation(void);
object_t make_closure(u16_t env, u16_t entry);

#endif // End of __LAMBDACHIP_VALUES_H__;
