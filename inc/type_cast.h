#ifndef __LAMBDACHIP_TYPE_CAST_H__
#define __LAMBDACHIP_TYPE_CAST_H__
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
#include "types.h"

#define BIT_ORDER_MASK  0xBAA50000
#define BIT_ORDER_MASK2 0xBAA5
#define BIT_ORDER_MASK3 0xBAA50001

void cast_imm_int_to_rational (object_t ret);
void cast_rational_to_imm_int_if_denominator_is_1 (object_t v);
void cast_rational_to_float (object_t v);
void cast_int_or_fractal_to_float (object_t v);

#endif /* __LAMBDACHIP_TYPE_CAST_H__ */