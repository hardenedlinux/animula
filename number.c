/*  Copyright (C) 2020-2025
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *        "Rafael Lee"                   <rafaellee.img@gmail.com>
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

#include "number.h"

typedef float (*real_op_t) (float);

static object_t op_dispatch (vm_t vm, object_t ret, immu_object_t x,
                             real_op_t real_op)
{
  switch (x->attr.type)
    {
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        f.f = real_op (f.f);
        ret->value = (void *)f.v;
        ret->attr.type = real;
        break;
      }
    case rational:
      {
        real_t f;
        f.v = (uintptr_t)cast_rational_to_float (x);
        f.f = real_op (f.f);
        ret->value = (void *)(imm_int_t)f.f;
        ret->attr.type = imm_int;
        break;
      }
    case imm_int:
      {
        *ret = *x;
        break;
      }
    default:
      PANIC ("Type not match, type is %d\n", x->attr.type);
    }

  return ret;
}

object_t _floor (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  return op_dispatch (vm, ret, x, floor);
}

object_t _floor_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  PANIC ("floor/ has not implemented yet\n");
  return NULL;
}

object_t _ceiling (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  return op_dispatch (vm, ret, x, ceil);
}

object_t _truncate (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

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

object_t _round (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);
  return op_dispatch (vm, ret, x, round);
}

object_t _rationalize (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _floor_quotient (vm_t vm, object_t ret, immu_object_t x,
                          immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _floor_remainder (vm_t vm, object_t ret, immu_object_t x,
                           immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _truncate_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _truncate_quotient (vm_t vm, object_t ret, immu_object_t x,
                             immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _truncate_remainder (vm_t vm, object_t ret, immu_object_t x,
                              immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _numerator (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _denominator (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _is_exact_integer (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _is_finite (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _is_infinite (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _is_nan (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _is_zero (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // xx is a number
  switch (x->attr.type)
    {
    case real:
    case imm_int:
      {
        *ret = x->value ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);
        break;
      }
    case rational:
      {
        imm_int_t check = (0xFFFF0000 & (imm_int_t)x->value);
        *ret = check ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);
        break;
      }
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    default:
      {
        PANIC ("Type not match, type is %d\n", x->attr.type);
      }
    }

  return ret;
}

object_t _is_positive (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  *ret
    = _int_gt (x, &zero) ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);

  return ret;
}

object_t _is_negative (vm_t vm, object_t ret, object_t x)
{
  VALIDATE_NUMBER (x);
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  *ret
    = _int_gt (x, &zero) ? GLOBAL_REF (false_const) : GLOBAL_REF (true_const);

  return ret;
}

bool __is_odd (vm_t vm, immu_object_t x, char *op)
{
  switch (x->attr.type)
    {
    case real:
      {
        Object tmp = {0};
        real_t a = (real_t)x->value;

        if (255 == a.exponent)
          {
            if (0 == a.mantissa)
              PANIC (op ": Wrong type argument - infinity!");
            else
              PANIC (op ": Wrong type argument - Nan!");
          }
        else if (0 == a.exponent)
          {
            if (0 == a.mantissa) // exactly 0
              *ret = GLOBAL_REF (false_const);
            else // a subnormal number
              PANIC (op ": Wrong type argument - %f", a.f);
          }
        else if (floorf (a.f) == a.f)
          {
            // an integer
            ret = a.mantissa & 1;
          }
        else
          {
            PANIC (op ": Wrong type argument - not an integer!");
          }

        break;
      }
    case imm_int:
      {
        ret = (uintptr_t)x->value & 1;
        break;
      }
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    case rational:
      {
        PANIC ("Rational not implemented yet!\n");
        break;
      }
    default:
      {
        PANIC ("Type not match, type is %d\n", x->attr.type);
      }
    }

  return ret;
}

bool _is_odd (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  *ret = __is_odd (vm, x, "odd?") ? GLOBAL_REF (true_const)
                                  : GLOBAL_REF (false_const);

  return ret;
}

object_t _is_even (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  *ret = __is_odd (vm, x, "even?") ? GLOBAL_REF (false_const)
                                   : GLOBAL_REF (true_const);

  return ret;
}

object_t _square (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _exact_integer_sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  PANIC ("Not implemented");
  return NULL;
}

object_t _expt (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  PANIC ("Not implemented");
  return NULL;
}
