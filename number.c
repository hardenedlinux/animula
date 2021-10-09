/*  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *        "Rafael Lee"                   <rafaellee.img@gmail.com>
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

#include "primitives.h"
#include "type_cast.h"
#ifdef LAMBDACHIP_ZEPHYR
#  include <drivers/gpio.h>
#  include <vos/drivers/gpio.h>
#endif /* LAMBDACHIP_ZEPHYR */
#include "lib.h"
#include "math.h"

object_t _floor (vm_t vm, object_t ret, object_t xx);
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

#ifdef LAMBDACHIP_ZEPHYR
#elif defined LAMBDACHIP_LINUX
#endif /* LAMBDACHIP_LINUX */

// extrenal declarations
extern bool _int_gt (object_t x, object_t y);
extern bool _int_lt (object_t x, object_t y);
extern bool _int_eq (object_t x, object_t y);

object_t _floor (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  // xx is a number
  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = floor (f.f);
      ret->value = (void *)f.v;
      ret->attr.type = real;
      return ret;
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      cast_int_or_fractal_to_float (x); // side effect, x is modified
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = floor (f.f);
      ret->value = (void *)(imm_int_t)f.f;
      ret->attr.type = imm_int;
      return ret;
    }
  else if (imm_int == x->attr.type)
    {
      return xx;
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _floor_div (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  VALIDATE_NUMBER (xx);
  VALIDATE_NUMBER (yy);

  PANIC ("floor/ has not implemented yet\n");
  return ret;
}

object_t _ceiling (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  // xx is a number
  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = ceil (f.f);
      ret->value = (void *)f.v;
      ret->attr.type = real;
      return ret;
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      cast_int_or_fractal_to_float (x); // side effect, x is modified
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = ceil (f.f);
      ret->value = (void *)(imm_int_t)f.f;
      ret->attr.type = imm_int;
      return ret;
    }
  else if (imm_int == x->attr.type)
    {
      return xx;
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _truncate (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};
  if (_int_gt (x, &zero)) // x > 0
    {
      return _floor (vm, ret, x);
    }
  else // x <= 0
    {
      return _ceiling (vm, ret, x);
    }
}

object_t _round (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = round (f.f);
      ret->value = (void *)f.v;
      ret->attr.type = real;
      return ret;
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      cast_int_or_fractal_to_float (x); // side effect, x is modified
      real_t f;
      f.v = (uintptr_t)x->value;
      f.f = round (f.f);
      imm_int_t z = (imm_int_t)f.f;
      ret->value = (void *)z;
      ret->attr.type = imm_int;
      return ret;
    }
  else if (imm_int == x->attr.type)
    {
      return xx;
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _rationalize (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _floor_quotient (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _floor_remainder (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _truncate_div (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _truncate_quotient (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _truncate_remainder (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _numerator (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _denominator (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _is_exact_integer (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _is_finite (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _is_infinite (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _is_nan (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _is_zero (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  // xx is a number
  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      real_t f;
      Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};
      f.f = 0.0;
      zero.value = (void *)f.v;
      if (_int_eq (&zero, x))
        {
          *ret = GLOBAL_REF (true_const);
          return ret;
        }
      else
        {
          *ret = GLOBAL_REF (false_const);
          return ret;
        }
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      if (0 == (0xFFFF0000 & (imm_int_t)x->value))
        {
          *ret = GLOBAL_REF (true_const);
          return ret;
        }
      else
        {
          *ret = GLOBAL_REF (false_const);
          return ret;
        }
    }
  else if (imm_int == x->attr.type)
    {
      Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};
      if (_int_eq (&zero, x))
        {
          *ret = GLOBAL_REF (true_const);
          return ret;
        }
      else
        {
          *ret = GLOBAL_REF (false_const);
          return ret;
        }
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _is_positive (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};
  if (_int_gt (xx, &zero))
    {
      *ret = GLOBAL_REF (true_const);
      return ret;
    }
  else
    {
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _is_negative (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};
  if (_int_lt (xx, &zero))
    {
      *ret = GLOBAL_REF (true_const);
      return ret;
    }
  else
    {
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _is_odd (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      if (_int_eq (x, _round (vm, ret, x)))
        {
          real_t a;
          a.v = (uintptr_t)x->value;
          imm_int_t b = (imm_int_t)a.f;
          if (b & 1) // LSB is 1
            {
              *ret = GLOBAL_REF (true_const);
              return ret;
            }
          else
            {
              *ret = GLOBAL_REF (false_const);
              return ret;
            }
          ret->attr.type = imm_int;
          return ret;
        }
      else
        {
          real_t a;
          a.v = (uintptr_t)x->value;
          PANIC ("Not an integer %f\n", a.f);
          return ret;
        }
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      PANIC ("Rational not implemented yet!\n");
      return ret;
    }
  else if (imm_int == x->attr.type)
    {
      imm_int_t z = (imm_int_t)x->value;
      if (z & 1) // LSB is 1
        {
          *ret = GLOBAL_REF (true_const);
          return ret;
        }
      else
        {
          *ret = GLOBAL_REF (false_const);
          return ret;
        }
      return xx;
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _is_even (vm_t vm, object_t ret, object_t xx)
{
  VALIDATE_NUMBER (xx);
  Object x_ = *xx;
  object_t x = &x_;

  if (complex_inexact == x->attr.type || complex_exact == x->attr.type)
    {
      PANIC ("Complex not implemented yet\n");
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
  else if (real == x->attr.type)
    {
      if (_int_eq (x, _round (vm, ret, x)))
        {
          real_t a;
          a.v = (uintptr_t)x->value;
          imm_int_t b = (imm_int_t)a.f;
          if (b & 1) // LSB is 1
            {
              *ret = GLOBAL_REF (false_const);
              return ret;
            }
          else
            {
              *ret = GLOBAL_REF (true_const);
              return ret;
            }
          ret->attr.type = imm_int;
          return ret;
        }
      else
        {
          real_t a;
          a.v = (uintptr_t)x->value;
          PANIC ("Not an integer %f\n", a.f);
          return ret;
        }
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type)
    {
      PANIC ("Rational not implemented yet!\n");
      return ret;
    }
  else if (imm_int == x->attr.type)
    {
      imm_int_t z = (imm_int_t)x->value;
      if (z & 1) // LSB is 1
        {
          *ret = GLOBAL_REF (false_const);
          return ret;
        }
      else
        {
          *ret = GLOBAL_REF (true_const);
          return ret;
        }
      return xx;
    }
  else
    {
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _square (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _sqrt (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _exact_integer_sqrt (vm_t vm, object_t ret, object_t xx)
{
  PANIC ("Not implemented");
  return ret;
}

object_t _expt (vm_t vm, object_t ret, object_t xx, object_t yy)
{
  PANIC ("Not implemented");
  return ret;
}