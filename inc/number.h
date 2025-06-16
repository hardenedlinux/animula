/*  Copyright (C) 2025
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.

 *  Animula is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "primitives.h"
#include "type_cast.h"
#ifdef ANIMULA_ZEPHYR
#  include <drivers/gpio.h>
#  include <vos/drivers/gpio.h>
#endif /* ANIMULA_ZEPHYR */
#include "lib.h"

object_t _floor (vm_t vm, object_t ret, imm_object_t x);
object_t _floor_div (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _ceiling (vm_t vm, object_t ret, object_t xx);
object_t _truncate (vm_t vm, object_t ret, object_t xx);
object_t _round (vm_t vm, object_t ret, object_t xx);
object_t _rationalize (vm_t vm, object_t ret, object_t xx);
object_t _floor_quotient (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _floor_remainder (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _truncate_div (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _truncate_quotient (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _truncate_remainder (vm_t vm, object_t ret, object_t xx, object_t yy);
object_t _numerator (vm_t vm, object_t ret, object_t xx);
object_t _denominator (vm_t vm, object_t ret, object_t xx);
object_t _is_exact_integer (vm_t vm, object_t ret, object_t xx);
object_t _is_finite (vm_t vm, object_t ret, object_t xx);
object_t _is_infinite (vm_t vm, object_t ret, object_t xx);
object_t _is_nan (vm_t vm, object_t ret, object_t xx);
object_t _is_zero (vm_t vm, object_t ret, object_t xx);
object_t _is_positive (vm_t vm, object_t ret, object_t xx);
object_t _is_negative (vm_t vm, object_t ret, object_t xx);
object_t _is_odd (vm_t vm, object_t ret, object_t xx);
object_t _is_even (vm_t vm, object_t ret, object_t xx);
object_t _square (vm_t vm, object_t ret, object_t xx);
object_t _sqrt (vm_t vm, object_t ret, object_t xx);
object_t _exact_integer_sqrt (vm_t vm, object_t ret, object_t xx);
object_t _expt (vm_t vm, object_t ret, object_t xx, object_t yy);

extern Object prim_number_p (object_t obj);

#ifdef ANIMULA_ZEPHYR
#elif defined ANIMULA_LINUX
#endif /* ANIMULA_LINUX */

// extrenal declarations
extern bool _int_gt (imm_object_t x, imm_object_t y);
extern bool _int_lt (imm_object_t x, imm_object_t y);
extern bool _int_eq (imm_object_t x, imm_object_t y);
