#ifndef __LAMBDACHIP_STRING_H__
#define __LAMBDACHIP_STRING_H__
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

#include "list.h"
#include "object.h"
#include "os.h"
#include "types.h"

bool str_eq (object_t s1, object_t s2);
object_t _read_char (vm_t vm, object_t ret);
object_t _read_str (vm_t vm, object_t ret, object_t cnt);
object_t _read_line (vm_t vm, object_t ret);
object_t _list_to_string (vm_t vm, object_t ret, object_t lst);
Object prim_string_p (object_t obj);
Object prim_char_p (object_t obj);
Object prim_keyword_p (object_t obj);

#endif // End of __LAMBDACHIP_STRING_H__
