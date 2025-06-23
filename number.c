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

typedef object_t (*real_op_t) (vm_t vm, object_t ret, immu_object_t x);
typedef object_t (*rational_op_t) (vm_t vm, object_t ret, immu_object_t x);

static object_t op_dispatch (vm_t vm, object_t ret, immu_object_t x,
                             real_op_t real_op, rational_op_t rational_op);
{
  switch (x->attr.type)
    {
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        *ret = GLOBAL_REF (false_const);
        return ret;
      }
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        f.f = real_op (f.f);
        ret->value = (void *)f.v;
        ret->attr.type = real;
        return ret;
      }
    case rational_pos:
    case rational_neg:
      {
        real_t f;
        f.v = (uintptr_t)cast_int_or_fractal_to_float (x);
        f.f = real_op (f.f);
        ret->value = (void *)(imm_int_t)f.f;
        ret->attr.type = imm_int;
        return ret;
      }
    case imm_int:
      {
        return x;
      }
    default:
      PANIC ("Type not match, type is %d\n", x->attr.type);
      *ret = GLOBAL_REF (false_const);
      return ret;
    }
}

object_t _floor (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  return op_dispatch (vm, ret, x, floor);
}

object_t _floor_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER (xx);
  VALIDATE_NUMBER (yy);

  PANIC ("floor/ has not implemented yet\n");
  return ret;
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
      return _floor (vm, ret, xx);
    }
  else // x <= 0
    {
      return _ceiling (vm, ret, xx);
    }
}

object_t _round (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);
  return op_dispatch (vm, ret, x, round);
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

static object_t __is_positive (vm_t vm, object_t ret, immu_object_t x)
{
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  if (_int_gt (x, &zero))
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

object_t _is_positive (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  return __is_positive (vm, ret, x);
}

object_t _is_negative (vm_t vm, object_t ret, object_t x)
{
  VALIDATE_NUMBER (x);

  return !__is_positive (vm, ret, x);
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
      real_t a;
      a.v = (uintptr_t)x->value;
      if (_int_eq (x, _round (vm, ret, x)))
        {
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

object_t _is_even (vm_t vm, object_t ret, immu_object_t xx)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        *ret = GLOBAL_REF (false_const);
        return ret;
      }
    case real:
      {
        real_t a;
        a.v = (uintptr_t)x->value;

        if (_int_eq (x, _round (vm, ret, x)))
          {
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
            PANIC ("Not an integer %f\n", a.f);
            return ret;
          }
      }
    case rational_pos:
    case rational_neg:
      {
        PANIC ("Rational not implemented yet!\n");
        *ret = GLOBAL_REF (false_const);
        return ret;
      }
    case imm_int:
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
      }
    default:
      {
        PANIC ("Type not match, type is %d\n", x->attr.type);
        *ret = GLOBAL_REF (false_const);
        return ret;
      }
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
