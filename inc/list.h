#ifndef __LAMBDACHIP_LIST_H__
#define __LAMBDACHIP_LIST_H__
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
#include "qlist.h"
#include "types.h"

object_t _car (vm_t vm, object_t ret, object_t obj);
object_t _cdr (vm_t vm, object_t ret, object_t obj);
object_t _cons (vm_t vm, object_t ret, object_t a, object_t b);
object_t _list_ref (vm_t vm, object_t ret, object_t lst, object_t idx);
object_t _list_set (vm_t vm, object_t ret, object_t lst, object_t idx,
                    object_t val);
object_t _list_append (vm_t vm, object_t ret, object_t l1, object_t l2);
object_t _list_length (vm_t vm, object_t ret, object_t l1);
void _list_sort_obj_ascending (obj_list_head_t head);
Object prim_list_p (object_t obj);
Object prim_pair_p (object_t obj);
Object prim_null_p (object_t obj);

#endif // End of __LAMBDACHIP_LIST_H__
