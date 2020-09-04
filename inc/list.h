#ifndef __LAMBDACHIP_LIST_H__
#define __LAMBDACHIP_LIST_H__
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
#include "types.h"

typedef void (*for_each_proc_t) (object_t);
typedef object_t (*map_proc_t) (object_t);

void list_for_each (object_t obj, for_each_proc_t proc);
void list_map (object_t obj, map_proc_t proc);

#endif // End of __LAMBDACHIP_LIST_H__
