#ifndef __LAMBDACHIP_LIB_H__
#define __LAMBDACHIP_LIB_H__
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

#include "types.h"
#include "debug.h"
#include "object.h"

s32_t _int_add(s32_t x, s32_t y);
s32_t _int_sub(s32_t x, s32_t y);
s32_t _int_mul(s32_t x, s32_t y);
s32_t _int_div(s32_t x, s32_t y);
void _object_print(object_t obj);

#endif // End of __LAMBDACHIP_LIB_H__
