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

#include "primitives.h"
#include "type_cast.h"
#ifdef LAMBDACHIP_ZEPHYR
// #  include <string.h> // memncpy
#  include <drivers/gpio.h>
#  include <vos/drivers/gpio.h>
#endif /* LAMBDACHIP_ZEPHYR */
#include "lib.h"

static bool _int_gt (object_t x, object_t y);

GLOBAL_DEF (prim_t, prim_table[PRIM_MAX]) = {0};

#define DIFF_EPSILON 0
// primitives implementation

static inline object_t _int_add (vm_t vm, object_t ret, object_t xx,
                                 object_t yy)
{
  Object x_ = *xx;
  Object y_ = *yy;
  object_t x = &x_;
  object_t y = &y_;
  if (complex_inexact == x->attr.type || complex_inexact == y->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == x->attr.type || complex_exact == y->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  else if (real == x->attr.type || real == y->attr.type)
    {
      real_t a, b;
      a.v = 0;
      b.v = 0;
      if (real == x->attr.type)
        {
          a.v = (uintptr_t) (x->value);
        }
      else if ((rational_pos == x->attr.type) || (rational_neg == x->attr.type))
        {
          int sign = (rational_pos == x->attr.type) ? 1 : -1;
          // FIXME: may lose precision
          a.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if (imm_int == x->attr.type)
        {
          // FIXME: may lose precision
          a.f = (float)(imm_int_t) (x->value);
        }
      else
        {
          PANIC ("Invalid type, expect %d or %d, but it's %d\n", imm_int, real,
                 x->attr.type);
        }

      if (real == y->attr.type)
        {
          b.v = (uintptr_t) (y->value);
        }
      else if ((rational_pos == y->attr.type) || (rational_neg == y->attr.type))
        {
          int sign = (rational_pos == y->attr.type) ? 1 : -1;
          b.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if ((imm_int == y->attr.type))
        {
          b.f = (float)(imm_int_t) (y->value);
        }
      else
        {
          PANIC ("Invalid type, expect %d or %d, but it's %d\n", imm_int, real,
                 y->attr.type);
        }
      real_t c;
      c.f = a.f + b.f;
      ret->attr.type = real;
      ret->value = (void *)c.v;
    }
  else if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type)
           || (rational_neg == y->attr.type) || (rational_pos == y->attr.type))
    {
      if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type))
        {
          if ((rational_neg == y->attr.type) || (rational_pos == y->attr.type))
            {
            }
          else if (imm_int == y->attr.type)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (y);
            }
          else
            {
              PANIC ("Type error, %d\n", y->attr.type);
            }
        }
      if ((rational_neg == y->attr.type) || (rational_pos == y->attr.type))
        {
          if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type))
            {
            }
          else if (imm_int == x->attr.type)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (x);
            }
          else
            {
              PANIC ("Type error, %d\n", x->attr.type);
            }
        }
      // TODO: check if s32 enough
      s64_t xd, xn, yd, yn, x_sign, y_sign;
      s64_t denominator, numerator, common_divisor;
      xn = ((imm_int_t) (x->value) >> 16) & 0xFFFF;
      xd = ((imm_int_t) (x->value) & 0xFFFF);
      x_sign = (rational_pos == x->attr.type) ? 1 : -1;
      yn = ((imm_int_t) (y->value) >> 16) & 0xFFFF;
      yd = ((imm_int_t) (y->value) & 0xFFFF);
      y_sign = (rational_pos == y->attr.type) ? 1 : -1;

      // a/b+c/d = (a*d+b*c)/(b*d)
      denominator = xd * yd; // safe, s32 * s32 is s64
      // denominator = xn*yn; // safe
      // safe, s32 * s32 + s32 * s32 is s64
      numerator = xn * yd * x_sign + yn * xd * y_sign;

      // only 32 bit is used, no overflow
      if (((numerator & 0xFFFFFFFF00000000) == 0)
          || ((numerator & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000))
        {
          common_divisor = gcd (denominator, numerator);
          denominator /= common_divisor;
          numerator /= common_divisor;
        }
      else
        {
          // side effect
          cast_rational_to_float (x);
          // side effect
          cast_rational_to_float (y);
          real_t a;
          real_t b;
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
          b.f = a.f + b.f;
          ret->attr.type = real;
          ret->value = (void *)b.v;
          return ret;
        }
      // gcd
      // if ((abs (denominator) <= 32678) && (abs (numerator) <= 32678))
      // check if only 16 bit LSB is effective
      // BOOL abs(int) cannot hold arguments with the type of s64_t
      if ((((numerator & 0xFFFFFFFFFFFF0000) == 0)
           || ((numerator & 0xFFFFFFFFFFFF0000) == 0xFFFFFFFFFFFF0000))
          && (((denominator & 0xFFFFFFFFFFFF0000) == 0)
              || ((denominator & 0xFFFFFFFFFFFF0000) == 0xFFFFFFFFFFFF0000)))
        {
          int sign = (denominator < 0) ? -1 : 1;
          sign = sign * ((numerator < 0) ? -1 : 1);

          // u16_t dd = abs (denominator) & 0xffff;
          // u16_t nn = abs (numerator) & 0xffff;
          u16_t dd = ((denominator < 0) ? -1 : 1) * denominator;
          u16_t nn = ((numerator < 0) ? -1 : 1) * numerator;
          // value of shift left is correct with signed int
          ret->value = (void *)((nn << 16) | dd);
          ret->attr.type = (sign > 0) ? rational_pos : rational_neg;
        }
      else
        {
          cast_rational_to_float (x);
          cast_rational_to_float (y);
          real_t a;
          real_t b;
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
          b.f = a.f / b.f;
          ret->attr.type = real;
          ret->value = (void *)b.v;
        }

      // ret->attr.type == (numerator>0)?rational_pos:rational_neg;
      // ret->value == ()
      return ret;
    }
  else if (imm_int == x->attr.type && imm_int == y->attr.type)
    {
      s64_t result = ((s64_t)x->value + (s64_t)y->value);
      s32_t result2 = 0xFFFFFFFF & result;
      real_t result3;
      if (result2 != result) // if overflow, convert to real
        {
          result3.f = (float)result;
          ret->attr.type = real;
          ret->value = (void *)(result3.v);
        }
      else
        {
          ret->value = (void *)result2;
          ret->attr.type = imm_int;
        }
      return ret;
    }
  else
    {
      PANIC ("Type error, x->attr.type == %d && y->attr.type == %d\n",
             x->attr.type, y->attr.type);
      return ret;
    }

  return ret;
}

static inline object_t _int_sub (vm_t vm, object_t ret, object_t xx,
                                 object_t yy)
{
  Object x_ = *xx;
  Object y_ = *yy;
  object_t x = &x_;
  object_t y = &y_;
  if (complex_inexact == xx->attr.type || complex_inexact == yy->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == xx->attr.type || complex_exact == yy->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  else if (real == y->attr.type)
    {
      real_t *pa;
      pa = (real_t *)(y->value);
      pa->f = -pa->f;
    }
  else if (rational_pos == y->attr.type)
    {
      y->attr.type = rational_neg;
    }
  else if (rational_neg == y->attr.type)
    {
      y->attr.type = rational_pos;
    }
  else if (imm_int == y->attr.type)
    {
      // FIXME:side effect
      if ((imm_int_t) (y->value) == MIN_INT32) // -1*2^31
        {
          PANIC ("Out of range, cannot calculate opposite number of %d\n",
                 (imm_int_t) (y->value));
        }
      else
        {
          y->value = (void *)(((imm_int_t) (y->value)) * -1);
        }
    }
  else
    {
      PANIC ("Invalid type, expect %d, %d, %d or %d, but it's %d\n", real,
             rational_pos, rational_neg, imm_int, y->attr.type);
    }
  return _int_add (vm, ret, x, y);
}

static inline object_t _int_mul (vm_t vm, object_t ret, object_t xx,
                                 object_t yy)
{
  Object x_ = *xx;
  Object y_ = *yy;
  object_t x = &x_;
  object_t y = &y_;
  if (complex_inexact == x->attr.type || complex_inexact == y->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == x->attr.type || complex_exact == y->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  else if (real == x->attr.type || real == y->attr.type)
    {
      real_t a, b;
      a.v = 0;
      b.v = 0;
      if (real == x->attr.type)
        {
          a.v = (uintptr_t) (x->value);
        }
      else if ((rational_pos == x->attr.type) || (rational_neg == x->attr.type))
        {
          int sign = (rational_pos == x->attr.type) ? 1 : -1;
          // FIXME: may lose precision
          a.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if (imm_int == x->attr.type)
        {
          // FIXME: may lose precision
          a.f = (float)(imm_int_t) (x->value);
        }
      else
        {
          PANIC ("Invalid type, expect %d or %d, but it's %d\n", imm_int, real,
                 x->attr.type);
        }

      if (real == y->attr.type)
        {
          b.v = (uintptr_t) (y->value);
        }
      else if ((rational_pos == y->attr.type) || (rational_neg == y->attr.type))
        {
          int sign = (rational_pos == y->attr.type) ? 1 : -1;
          b.f = sign * (float)(((imm_int_t) (y->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (y->value) & 0xFFFF);
        }
      else if ((imm_int == y->attr.type))
        {
          // FIXME: may lose precision
          b.f = (float)(imm_int_t) (y->value);
        }
      else
        {
          PANIC ("Invalid type, expect %d or %d, but it's %d\n", imm_int, real,
                 y->attr.type);
        }
      real_t c;
      c.f = a.f * b.f;
      ret->attr.type = real;
      ret->value = (void *)c.v;
    }
  else if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type)
           || (rational_neg == y->attr.type) || (rational_pos == y->attr.type))
    {
      if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type))
        {
          if ((rational_neg == y->attr.type) || (rational_pos == y->attr.type))
            {
            }
          else if (imm_int == y->attr.type)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (y);
            }
          else
            {
              PANIC ("Type error, %d\n", y->attr.type);
            }
        }
      if ((rational_neg == y->attr.type) || (rational_pos == y->attr.type))
        {
          if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type))
            {
            }
          else if (imm_int == x->attr.type)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (x);
            }
          else
            {
              PANIC ("Type error, %d\n", x->attr.type);
            }
        }
      // TODO: check if s32 enough
      s64_t xd, xn, yd, yn, x_sign, y_sign;
      s64_t denominator, numerator, common_divisor;
      xn = ((imm_int_t) (x->value) >> 16) & 0xFFFF;
      xd = ((imm_int_t) (x->value) & 0xFFFF);
      x_sign = (rational_pos == x->attr.type) ? 1 : -1;
      yn = ((imm_int_t) (y->value) >> 16) & 0xFFFF;
      yd = ((imm_int_t) (y->value) & 0xFFFF);
      // FIXME: if integer canont convert to rational
      y_sign = (rational_pos == y->attr.type) ? 1 : -1;

      // a/b*c/d = (a*c)/(b*d)
      denominator = xd * yd; // safe, s32 * s32 is s64
      // safe, s32 * s32 + s32 * s32 is s64
      // 65535*65535 = 2^32-2*65536+1
      numerator = xn * x_sign * yn * y_sign;

      // only 32 bit is used, no overflow
      if (((numerator & 0xFFFFFFFF00000000) == 0)
          || ((numerator & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000))
        {
          common_divisor = gcd (denominator, numerator);
          denominator /= common_divisor;
          numerator /= common_divisor;
        }
      else // integer more than 32 bit, convert to real
        {
          // side effect
          cast_rational_to_float (x);
          // side effect
          cast_rational_to_float (y);
          real_t a, b;
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
          b.f = a.f * b.f;
          ret->attr.type = real;
          ret->value = (void *)b.v;
          return ret;
        }
      // gcd
      // if ((abs (denominator) <= 32678) && (abs (numerator) <= 32678))
      // check if only 16 bit LSB is effective
      // BOOL abs(int) cannot hold arguments with the type of s64_t
      if ((((numerator & 0xFFFFFFFFFFFF0000) == 0)
           || ((numerator & 0xFFFFFFFFFFFF0000) == 0xFFFFFFFFFFFF0000))
          && (((denominator & 0xFFFFFFFFFFFF0000) == 0)
              || ((denominator & 0xFFFFFFFFFFFF0000) == 0xFFFFFFFFFFFF0000)))
        {
          int sign = (denominator < 0) ? -1 : 1;
          sign = sign * ((numerator < 0) ? -1 : 1);

          // u16_t dd = abs (denominator) & 0xffff;
          // u16_t nn = abs (numerator) & 0xffff;
          u16_t dd = ((denominator < 0) ? -1 : 1) * denominator;
          u16_t nn = ((numerator < 0) ? -1 : 1) * numerator;
          // value of shift left is correct with signed int
          ret->value = (void *)((nn << 16) | dd);
          ret->attr.type = (sign > 0) ? rational_pos : rational_neg;
          cast_rational_to_imm_int_if_denominator_is_1 (ret);
          return ret;
        }
      else // more than 16 bit is used, cannot use as rational, convert to float
        {
          // convert to float
          cast_rational_to_float (x);
          cast_rational_to_float (y);
          real_t a, b;
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
          b.f = a.f * b.f;
          ret->attr.type = real;
          ret->value = (void *)b.v;
        }

      // ret->attr.type == (numerator>0)?rational_pos:rational_neg;
      // ret->value == ()
      return ret;
    }
  else if (imm_int == x->attr.type && imm_int == y->attr.type)
    {
      s64_t result = ((s64_t)x->value * (s64_t)y->value);
      s32_t result2 = 0xFFFFFFFF & result;
      real_t result3;
      if (result2 != result) // if overflow
        {
          // PANIC ("Add overflow or underflow %lld, %d\n", result, result2);
          result3.f = (float)result;
          ret->value = (void *)result3.v;
          ret->attr.type = real;
        }
      else
        {
          ret->value = (void *)result2;
          ret->attr.type = imm_int;
        }
      return ret;
    }
  else
    {
      PANIC ("Type error, x->attr.type == %d && y->attr.type == %d\n",
             x->attr.type, y->attr.type);
      return ret;
    }

  return ret;
}

static inline object_t _int_div (vm_t vm, object_t ret, object_t xx,
                                 object_t yy)
{
  Object x_ = *xx;
  Object y_ = *yy;
  object_t x = &x_;
  object_t y = &y_;
  if (complex_inexact == x->attr.type || complex_inexact == y->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == x->attr.type || complex_exact == y->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  else if (real == x->attr.type || real == y->attr.type)
    {
      real_t a, b;
      cast_int_or_fractal_to_float (x);
      cast_int_or_fractal_to_float (y);
      a.v = (uintptr_t)x->value;
      b.v = (uintptr_t)y->value;
      if (b.f != 0.0f)
        {
          b.f = a.f / b.f;
        }
      else
        {
          PANIC ("Div by 0 error!\n");
        }
      ret->value = (void *)b.v;
      ret->attr.type = real;
    }
  else if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type)
           || (rational_neg == y->attr.type) || (rational_pos == y->attr.type))
    {
      imm_int_t nx = 0;
      imm_int_t dx = 0;
      imm_int_t ny = 0;
      imm_int_t dy = 0;
      imm_int_t sign_x = 0;
      imm_int_t sign_y = 0;
      imm_int_t sign;
      s64_t nn = 0;
      s64_t dd = 0;
      s64_t common_divisor64 = 0;

      if ((rational_pos == x->attr.type) || (rational_neg == x->attr.type))
        {
          nx = (((imm_int_t) (x->value)) >> 16) & 0xFFFF;
          dx = ((imm_int_t) (x->value)) & 0xFFFF;
          sign_x = (rational_pos == x->attr.type) ? 1 : -1;
        }
      else if (imm_int == x->attr.type)
        {
          nx = (imm_int_t) (x->value);
          dx = 1;
          sign_x = (nx >= 0) ? 1 : -1;
          nx = abs (nx);
        }
      else
        {
          PANIC ("Invalid type, expect %d, %d or %d, but it's %d\n",
                 rational_pos, rational_neg, imm_int, x->attr.type);
        }
      if ((rational_pos == y->attr.type) || (rational_neg == y->attr.type))
        {
          ny = (((imm_int_t) (y->value)) >> 16) & 0xFFFF;
          dy = ((imm_int_t) (y->value)) & 0xFFFF;
          sign_y = (rational_pos == y->attr.type) ? 1 : -1;
        }
      else if (imm_int == y->attr.type)
        {
          ny = (imm_int_t) (y->value);
          dy = 1;
          sign_y = (ny >= 0) ? 1 : -1;
          ny = abs (ny);
        }
      else
        {
          PANIC ("Invalid type, expect %d, %d or %d, but it's %d\n",
                 rational_pos, rational_neg, imm_int, x->attr.type);
        }
      nn = nx * dy;
      dd = dx * ny;
      nn = abs64 (nn);
      dd = abs64 (dd);
      sign = sign_x * sign_y;
      common_divisor64 = gcd64 (nn, dd);
      nn = nn / common_divisor64;
      dd = dd / common_divisor64;
      if (0 == dd)
        {
          PANIC ("Div by 0 error!\n");
        }
      if (nn < MIN_REAL_DENOMINATOR || nn > MAX_REAL_DENOMINATOR
          || dd < MIN_REAL_DENOMINATOR || dd > MAX_REAL_DENOMINATOR)
        {
          real_t a;
          a.f = (float)(nn * sign) / (float)dd;
          ret->value = (void *)a.v;
          ret->attr.type = real;
        }
      else // nn and dd are in range
        {
          ret->value = (void *)((nn << 16) | dd);
          ret->attr.type = (sign >= 0) ? rational_pos : rational_neg;
          cast_rational_to_imm_int_if_denominator_is_1 (ret);
        }
    }
  // int / int
  else if (imm_int == x->attr.type && imm_int == y->attr.type)
    {
      imm_int_t n = (imm_int_t) (x->value);
      imm_int_t d = (imm_int_t) (y->value);
      imm_int_t sign = (n >= 0) ? 1 : -1;
      sign = sign * ((d >= 0) ? 1 : -1);
      n = abs (n);
      d = abs (d);

      if (0 == d)
        {
          PANIC ("Div by 0 error!\n");
        }
      imm_int_t common_divisor = gcd (n, d);
      n = n / common_divisor;
      d = d / common_divisor;

      if (n < MIN_REAL_DENOMINATOR || n > MAX_REAL_DENOMINATOR
          || d < MIN_REAL_DENOMINATOR || d > MAX_REAL_DENOMINATOR)
        {
          ret->attr.type = real;
          real_t c;
          c.f = sign * (float)n / (float)d;
          ret->value = (void *)c.v;
        }
      else
        {
          if (sign > 0)
            {
              ret->attr.type = rational_pos;
            }
          else
            {
              ret->attr.type = rational_neg;
            }
          ret->value = (void *)((n << 16) | d);
          cast_rational_to_imm_int_if_denominator_is_1 (ret);
        }
    }
  else
    {
      PANIC ("Type error, x->attr.type == %d && y->attr.type == %d\n",
             x->attr.type, y->attr.type);
      return ret;
    }

  return ret;
}

imm_int_t _int_modulo (imm_int_t x, imm_int_t y)
{
  imm_int_t m = x % y;
  return y < 0 ? (m <= 0 ? m : m + y) : (m >= 0 ? m : m + y);
}

static inline imm_int_t _int_remainder (imm_int_t x, imm_int_t y)
{
  return x % y;
}

void _object_print (object_t obj)
{
  object_printer (obj);
}

static bool _int_eq (object_t xx, object_t yy)
{
  Object x_ = *xx;
  Object y_ = *yy;
  object_t x = &x_;
  object_t y = &y_;

  if (complex_inexact == x->attr.type || complex_inexact == y->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == x->attr.type || complex_exact == y->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  else if (real == x->attr.type || real == y->attr.type)
    {
      real_t a = {0};
      real_t b = {0};
      if (rational_pos == x->attr.type || rational_neg == x->attr.type
          || imm_int == x->attr.type)
        {
          cast_int_or_fractal_to_float (x);
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
        }
      if (rational_pos == y->attr.type || rational_neg == y->attr.type
          || imm_int == y->attr.type)
        {
          cast_int_or_fractal_to_float (y);
          a.v = (uintptr_t)x->value;
          b.v = (uintptr_t)y->value;
        }
      // since the decimal point is floated in the float point number
      // There will be
      if (os_fabs (a.f - b.f) <= DIFF_EPSILON)
        {
          return true;
        }
      else
        {
          return false;
        }
    }
  else if (rational_pos == x->attr.type || rational_neg == x->attr.type
           || rational_pos == y->attr.type || rational_neg == y->attr.type)
    {
      if (rational_pos == x->attr.type || rational_neg == x->attr.type)
        {
          cast_imm_int_to_rational (y);
        }

      if (rational_pos == y->attr.type || rational_neg == y->attr.type)
        {
          cast_imm_int_to_rational (x);
        }

      if (y->attr.type == x->attr.type)
        {
          if (x->value == y->value)
            {
              PANIC ("No formalized rational number is equal to an integer!\n");
              return true;
            }
          else
            {
              return false;
            }
        }
      else
        {
          PANIC ("Program shall not reach here!\n");
        }
    }
  else if (imm_int == x->attr.type && imm_int == y->attr.type)
    {
      if (x->value == y->value)
        {
          return true;
        }
      else // type error
        {
          return false;
        }
    }
  else
    {
      PANIC ("Type not supported, x type = %d, y type = %d!\n", xx->attr.type,
             yy->attr.type);
    }
  PANIC ("Program shall not reach here!\n");
  return false;
}

static bool _int_lt (object_t x, object_t y)
{
  if (false == (_int_eq (x, y)) && false == (_int_gt (x, y)))
    {
      return true;
    }
  return false;
}

static bool _int_gt (object_t x, object_t y)
{
  if (complex_inexact == x->attr.type || complex_inexact == y->attr.type)
    {
      PANIC ("Complex_inexact is not supported yet!\n");
    }
  else if (complex_exact == x->attr.type || complex_exact == y->attr.type)
    {
      PANIC ("Complex_exact is not supported yet!\n");
    }
  if (real == x->attr.type || real == y->attr.type)
    {
      cast_int_or_fractal_to_float (x);
      cast_int_or_fractal_to_float (y);
      real_t a, b;
      a.f = 0.0;
      b.f = 0.0;
      a.v = (uintptr_t)x->value;
      b.v = (uintptr_t)y->value;
      return a.f > b.f;
    }
  else if ((rational_neg == x->attr.type) || (rational_pos == x->attr.type)
           || (rational_neg == y->attr.type) || (rational_pos == y->attr.type))
    {
      cast_imm_int_to_rational (x);
      cast_imm_int_to_rational (y);
      s64_t x_n = 0;
      s64_t x_d = 0;
      s64_t x_sign = 0;
      s64_t y_n = 0;
      s64_t y_d = 0;
      s64_t y_sign = 0;

      x_n = ((imm_int_t) (x->value) >> 16) & 0xFFFF;
      x_d = ((imm_int_t) (x->value) & 0xFFFF);
      x_sign = (rational_pos == x->attr.type) ? 1 : -1;
      y_n = ((imm_int_t) (y->value) >> 16) & 0xFFFF;
      y_d = ((imm_int_t) (y->value) & 0xFFFF);
      y_sign = (rational_pos == y->attr.type) ? 1 : -1;

      x_n = x_sign * x_n * y_d;
      y_n = y_sign * y_n * x_d;
      return x_n > y_n;
    }
  else if (imm_int == x->attr.type && imm_int == y->attr.type)
    {
      return (imm_int_t)x->value > (imm_int_t)y->value;
    }
  else
    {
      PANIC ("Type not supported, x->attr.type = %d, y->attr.type = %d\n",
             x->attr.type, y->attr.type);
      return true;
    }
  PANIC ("Code should not run here, x->attr.type = %d, y->attr.type = %d\n",
         x->attr.type, y->attr.type);
  return true;
}

static bool _int_le (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value <= (imm_int_t)y->value;
}

static bool _int_ge (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value >= (imm_int_t)y->value;
}
// --------------------------------------------------

static bool _not (object_t obj)
{
  return is_false (obj);
}

static bool _eq (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;

  if (t1 != t2)
    {
      return false;
    }
  else if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
    {
      return true;
    }
  else if (procedure == t1)
    {
      return (a->proc.entry == b->proc.entry);
    }

  return (a->value == b->value);
}

static bool _eqv (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
      /* case record: */
      /* case bytevector: */
    case rational_pos:
    case rational_neg:
    case imm_int:
      {
        ret = _int_eq (a, b);
        break;
      }
    case symbol:
      {
        ret = symbol_eq (a, b);
        break;
      }
    case boolean:
      {
        ret = (a == b);
        break;
      }
    case pair:
    case string:
    case vector:
    case list:
      {
        if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
          {
            ret = true;
          }
        else
          {
            /* NOTE:
             * For collections, eqv? only compare their head pointer.
             */
            ret = (a->value == b->value);
          }
        break;
      }
    case procedure:
      {
        /* NOTE:
         * R7RS demands the procedure that has side-effects for
         * different behaviour or return values to be the different
         * object. This is guaranteed by the compiler, not by the VM.
         */
        ret = (a->proc.entry == b->proc.entry);
        break;
      }
    default:
      {
        PANIC ("eqv?: The type %d hasn't implemented yet\n", t1);
      }
      // TODO-1
      /* case character: */
      /*   { */
      /*     ret = character_eq (a, b); */
      /*     break; */
      /*   } */

      // TODO-2
      // Inexact numbers
    }

  return ret;
}

static bool _equal (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
    case pair:
      {
        pair_t ap = (pair_t)a->value;
        pair_t bp = (pair_t)b->value;
        ret = (_equal (ap->car, bp->car) && _equal (ap->cdr, bp->cdr));
        break;
      }
    case string:
      {
        ret = str_eq (a, b);
        break;
      }
    case list:
      {
        if (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b))
          {
            ret = true;
          }
        else
          {
            ListHead *h1 = LIST_OBJECT_HEAD (a);
            ListHead *h2 = LIST_OBJECT_HEAD (b);
            list_node_t n1 = NULL;
            list_node_t n2 = SLIST_FIRST (h2);

            SLIST_FOREACH (n1, h1, next)
            {
              ret = _equal (n1->obj, n2->obj);
              if (!ret)
                break;
              n2 = SLIST_NEXT (n2, next);
            }
          }
        break;
      }
    case vector:
      {
        PANIC ("equal?: vector hasn't been implemented yet!\n");
        /* TODO: iterate each element and call equal? */
        break;
      }
      /* case bytevector: */
      /*   { */
      /*     ret = bytevector_eq (a, b); */
      /*     break; */
      /*   } */
    default:
      {
        ret = _eqv (a, b);
        break;
      }
    }

  return ret;
}

static object_t _os_usleep (vm_t vm, object_t ret, object_t us)
{
  VALIDATE (us, imm_int);

  os_usleep ((int32_t)us->value);
  *ret = GLOBAL_REF (none_const);
  return ret;
}

#ifdef LAMBDACHIP_ZEPHYR

extern GLOBAL_DEF (super_device, super_dev_led0);
extern GLOBAL_DEF (super_device, super_dev_led1);
extern GLOBAL_DEF (super_device, super_dev_led2);
extern GLOBAL_DEF (super_device, super_dev_led3);

extern GLOBAL_DEF (super_device, super_dev_gpio_pa9);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa10);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb4);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa8);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb5);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb6);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb7);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb8);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb9);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa2);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa3);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb3);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb10);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb15);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb14);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb13);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb12);
extern GLOBAL_DEF (super_device, super_dev_gpio_ble_disable);

extern GLOBAL_DEF (super_device, super_dev_i2c2);
extern GLOBAL_DEF (super_device, super_dev_i2c3);

static object_t _os_get_board_id (vm_t vm, object_t ret)
{
  static uint32_t g_board_uid[3] = {0, 0, 0};
  ret->attr.type = mut_string;
  char *uid = (char *)GC_MALLOC (BOARD_ID_LEN); // last is \0, shall be included
  ret->value = (void *)uid;

  /* copy 96 bit UID as 3 uint32_t integer
   * then convert to 24 bytes of string
   */
  if (0 == g_board_uid[0])
    {
      os_memcpy (g_board_uid, (char *)UID_BASE, sizeof (g_board_uid));
    }
  os_snprintk (uid, BOARD_ID_LEN, "%08X%08X%08X", g_board_uid[0],
               g_board_uid[1], g_board_uid[2]);

  return ret;
}

extern const struct device *GLOBAL_REF (dev_led0);
extern const struct device *GLOBAL_REF (dev_led1);
extern const struct device *GLOBAL_REF (dev_led2);
extern const struct device *GLOBAL_REF (dev_led3);

static super_device *translate_supper_dev_from_symbol (object_t sym)
{
  VALIDATE (sym, symbol);
  super_device *ret = NULL;
  static const char char_dev_led0[] = "dev_led0";
  static const char char_dev_led1[] = "dev_led1";
  static const char char_dev_led2[] = "dev_led2";
  static const char char_dev_led3[] = "dev_led3";
  static const char char_dev_gpio_pa9[] = "dev_gpio_pa9";
  static const char char_dev_gpio_pa10[] = "dev_gpio_pa10";
  static const char char_dev_gpio_pb4[] = "dev_gpio_pb4";
  static const char char_dev_gpio_pa8[] = "dev_gpio_pa8";
  static const char char_dev_gpio_pb5[] = "dev_gpio_pb5";
  static const char char_dev_gpio_pb6[] = "dev_gpio_pb6";
  static const char char_dev_gpio_pb7[] = "dev_gpio_pb7";
  static const char char_dev_gpio_pb8[] = "dev_gpio_pb8";
  static const char char_dev_gpio_pb9[] = "dev_gpio_pb9";
  static const char char_dev_gpio_pa2[] = "dev_gpio_pa2";
  static const char char_dev_gpio_pa3[] = "dev_gpio_pa3";
  static const char char_dev_gpio_pb3[] = "dev_gpio_pb3";
  static const char char_dev_gpio_pb10[] = "dev_gpio_pb10";
  static const char char_dev_gpio_pb15[] = "dev_gpio_pb15";
  static const char char_dev_gpio_pb14[] = "dev_gpio_pb14";
  static const char char_dev_gpio_pb13[] = "dev_gpio_pb13";
  static const char char_dev_gpio_pb12[] = "dev_gpio_pb12";
  static const char char_dev_gpio_ble_disable[] = "dev_gpio_ble_disable";
  static const char char_dev_i2c2[] = "dev_i2c2";
  static const char char_dev_i2c3[] = "dev_i2c3";

  const char *str_buf = (const char *)sym->value;
  size_t len = os_strnlen (str_buf, MAX_STR_LEN);
  if (len < 5)
    goto PANIC;
  if (0 != os_strncmp (&(str_buf[0]), &(char_dev_led0[0]), 4))
    goto PANIC;
  switch (str_buf[4])
    {
    case 'l':
      if (len < 8)
        goto PANIC;
      if (0 != os_strncmp (&(str_buf[5]), &(char_dev_led0[5]), 2))
        goto PANIC;
      switch (str_buf[7])
        {
        case '0':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_led0));
          break;
        case '1':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_led1));
          break;
        case '2':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_led2));
          break;
        case '3':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_led3));
          break;
        default:
          break;
        }
      break;
    case 'g':
      if (len < 10)
        goto PANIC;
      if (0 != os_strncmp (&(str_buf[5]), &(char_dev_gpio_pa9[5]), 4))
        goto PANIC;
      switch (str_buf[9])
        {
        case 'p':
          if (len < 11)
            goto PANIC;
          switch (str_buf[10])
            {
            case 'a':
              if (len < 12)
                goto PANIC;
              switch (str_buf[11])
                {
                case '9':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pa9));
                  break;
                case '1':
                  if (len != 13)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pa10));
                  break;
                case '8':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pa8));
                  break;
                case '2':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pa2));
                  break;
                case '3':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pa3));
                  break;
                default:
                  break;
                }
              break;
            case 'b':
              if (len < 12)
                goto PANIC;
              switch (str_buf[11])
                {
                case '4':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb4));
                  break;
                case '5':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb5));
                  break;
                case '6':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb6));
                  break;
                case '7':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb7));
                  break;
                case '8':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb8));
                  break;
                case '9':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb9));
                  break;
                case '3':
                  if (len != 12)
                    goto PANIC;
                  ret = &(GLOBAL_REF (super_dev_gpio_pb3));
                  break;
                case '1':
                  if (len < 13)
                    goto PANIC;
                  switch (str_buf[12])
                    {
                    case '0':
                      if (len != 13)
                        goto PANIC;
                      ret = &(GLOBAL_REF (super_dev_gpio_pb10));
                      break;
                    case '5':
                      if (len != 13)
                        goto PANIC;
                      ret = &(GLOBAL_REF (super_dev_gpio_pb15));
                      break;
                    case '4':
                      if (len != 13)
                        goto PANIC;
                      ret = &(GLOBAL_REF (super_dev_gpio_pb14));
                      break;
                    case '3':
                      if (len != 13)
                        goto PANIC;
                      ret = &(GLOBAL_REF (super_dev_gpio_pb13));
                      break;
                    case '2':
                      if (len != 13)
                        goto PANIC;
                      ret = &(GLOBAL_REF (super_dev_gpio_pb12));
                      break;
                    default:
                      break;
                    }
                  break;
                default:
                  break;
                }
              break;
            default:
              break;
            }
          break;
        case 'b':
          if (len != 20)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_gpio_ble_disable));
          break;
        default:
          break;
        }
      break;
    case 'i':
      if (len < 8)
        goto PANIC;
      if (0 != os_strncmp (&(str_buf[5]), &(char_dev_i2c2[5]), 2))
        goto PANIC;
      switch (str_buf[7])
        {
        case '2':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_i2c2));
          break;
        case '3':
          if (len != 8)
            goto PANIC;
          ret = &(GLOBAL_REF (super_dev_i2c3));
          break;
        default:
          break;
        }
      break;
    default:
      break;
    }

  goto OK;
PANIC:
  PANIC ("BUG: Invalid symbol name %s!\n", str_buf);
OK:
  return ret;
}

static object_t _os_device_configure (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE (obj, symbol);

  *ret = GLOBAL_REF (none_const);
  // const char *str_buf = GET_SYBOL ((u32_t)obj->value);
  super_device *p = translate_supper_dev_from_symbol (obj);

  // FIXME: flags for different pin shall be different
  if (SUPERDEVICE_TYPE_GPIO_PIN == p->type)
    {
      gpio_pin_configure (p->dev, p->gpio_pin, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    }
  else if (SUPERDEVICE_TYPE_I2C == p->type)
    {
      i2c_configure (p->dev, 0);
    }
  else if (SUPERDEVICE_TYPE_SPI == p->type)
    {
    }
  else
    {
      PANIC ("device type not defined, cannot handle");
    }

  // os_printk ("imm_int_t _os_gpio_toggle (%s)\n", str_buf);
  return ret;
}

// dev->value is the string/symbol refer to of a super_device
static object_t _os_gpio_set (vm_t vm, object_t ret, object_t dev, object_t v)
{
  VALIDATE (dev, symbol);
  VALIDATE (v, boolean);

  super_device *p = translate_supper_dev_from_symbol (dev);
  gpio_pin_set (p->dev, p->gpio_pin, (int)v->value);
  *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_toggle (vm_t vm, object_t ret, object_t dev)
{
  VALIDATE (dev, symbol);
  super_device *p = translate_supper_dev_from_symbol (dev);
  gpio_pin_toggle (p->dev, p->gpio_pin);
  *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_byte (vm_t vm, object_t ret, object_t dev,
                                   object_t dev_addr, object_t reg_addr)
{
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  super_device *p = translate_supper_dev_from_symbol (dev);
  uint8_t buf_read[2] = {0, 0};
  int status = i2c_reg_read_byte (p->dev, (imm_int_t)dev_addr->value,
                                  (imm_int_t)reg_addr->value, buf_read);
  if (0 == status)
    {
      ret->value = (void *)buf_read[0];
    }
  else
    {
      *ret = GLOBAL_REF (false_const);
    }
  return ret;
}

static object_t _os_i2c_write_byte (vm_t vm, object_t ret, object_t dev,
                                    object_t dev_addr, object_t reg_addr,
                                    object_t value)
{
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  VALIDATE (value, imm_int);
  super_device *p = translate_supper_dev_from_symbol (dev);
  uint8_t tx_buf[2] = {(imm_int_t)reg_addr->value, (imm_int_t)value->value};
  int status
    = i2c_reg_write_byte (p->dev, (imm_int_t)dev_addr->value,
                          (imm_int_t)reg_addr->value, (imm_int_t)value->value);
  if (status != 0)
    *ret = GLOBAL_REF (false_const);
  else
    *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_list (vm_t vm, object_t ret, object_t dev,
                                   object_t i2c_addr, object_t length)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
  super_device *p = translate_supper_dev_from_symbol (dev);

  imm_int_t len_list = (imm_int_t)length->value;
  uint8_t *rx_buf = (uint8_t *)GC_MALLOC (len_list);
  if (!rx_buf)
    {
      ret->attr.type = boolean;
      ret->value = &GLOBAL_REF (false_const);
      os_printk ("%s, %s: GC_MALLOC fail\n", __FILE__, __FUNCTION__);
      return ret;
    }

  int status = i2c_read (p->dev, rx_buf, len_list, (imm_int_t)i2c_addr->value);
  if (status != 0)
    {
      *ret = GLOBAL_REF (false_const);
      os_free (rx_buf);
      rx_buf = (void *)NULL;
      return ret;
    }

  list_t l = NEW_INNER_OBJ (list);
  SLIST_INIT (&l->list);
  ret->attr.type = list;
  ret->attr.gc = 1;
  ret->value = (void *)l;

  list_node_t iter = NULL;
  for (imm_int_t i = 0; i < len_list; i++)
    {
      object_t new_obj = NEW_OBJ (0);

      // FIXME: check if pop is needed?
      // *new_obj = (object_t)POP_OBJ ();
      list_node_t bl = NEW_LIST_NODE ();
      bl->obj = new_obj;
      bl->obj->value = (void *)rx_buf[i];
      if (0 == i)
        {
          SLIST_INSERT_HEAD (&l->list, bl, next);
          iter = bl;
        }
      else
        {
          SLIST_INSERT_AFTER (iter, bl, next);
          iter = bl;
        }
    }

  os_free (rx_buf);
  rx_buf = (void *)NULL;
  return ret;
}

static object_t _os_i2c_write_list (vm_t vm, object_t ret, object_t dev,
                                    object_t i2c_addr, object_t lst)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (lst, list);
  super_device *p = translate_supper_dev_from_symbol (dev);

  Object len;
  object_t len_p = &len;
  // side effect
  len_p = _list_length (vm, len_p, lst);
  imm_int_t len_list = (imm_int_t)len_p->value;

  uint8_t *tx_buf = (uint8_t *)GC_MALLOC (len_list);
  if (!tx_buf)
    {
      ret->attr.type = boolean;
      ret->value = &GLOBAL_REF (false_const);
      PANIC ("Malloc failed!\n");
      return ret;
    }

  list_t obj_lst = (list_t) (lst->value);
  ListHead head = obj_lst->list;
  list_node_t iter = {0};
  imm_int_t index = 0;
  SLIST_FOREACH (iter, &head, next)
  {
    tx_buf[index] = (imm_int_t)iter->obj->value;
    index++;
  }

  int status = i2c_write (p->dev, tx_buf, len_list, (imm_int_t)i2c_addr->value);

  os_free (tx_buf);
  tx_buf = (void *)NULL;
  if (status != 0)
    *ret = GLOBAL_REF (false_const);
  else
    *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_bytevector (vm_t vm, object_t ret, object_t dev,
                                         object_t i2c_addr, object_t length)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
  super_device *p = translate_supper_dev_from_symbol (dev);

  imm_int_t len_list = (imm_int_t)length->value;
  uint8_t *buf = (uint8_t *)GC_MALLOC (len_list);
  if (!buf)
    {
      ret->attr.type = boolean;
      ret->value = (object_t)&GLOBAL_REF (false_const);
      PANIC ("GC_MALLOC fail\n");
      return ret;
    }

  int status = i2c_read (p->dev, buf, len_list, (imm_int_t)i2c_addr->value);
  if (status != 0)
    {
      // FIXME: ret is created outside, it may have memory leak here
      *ret = GLOBAL_REF (false_const);
      // ret->attr.type = boolean;
      // ret->value = (object_t)&GLOBAL_REF (false_const);
      os_free (buf);
      buf = (void *)NULL;
      return ret;
    }

  mut_bytevector_t bv = NEW_INNER_OBJ (mut_bytevector);
  bv->attr.gc = PERMANENT_OBJ; // avoid unexpected collection by GC before done
  bv->vec = buf;
  ret->attr.type = mut_bytevector;
  ret->attr.gc = PERMANENT_OBJ;
  ret->value = (void *)bv;

  ret->attr.gc = GEN_1_OBJ;
  bv->attr.gc = GEN_1_OBJ;
  // the ownership of buf is transfered to ret->bv->vec
  // do not need to free here
  return ret;
}

static object_t _os_spi_transceive (vm_t vm, object_t ret, object_t dev,
                                    object_t spi_config, object_t send_buffer,
                                    object_t receive_buffer)
{
  VALIDATE (dev, symbol);
  VALIDATE (spi_config, list);
  VALIDATE (send_buffer, list);
  VALIDATE (receive_buffer, list);
  // super_device *p = translate_supper_dev_from_symbol (dev);

  object_printer (spi_config);
  int status = 0;
  if (status != 0)
    *ret = GLOBAL_REF (false_const);
  else
    *ret = GLOBAL_REF (none_const);
  return ret;
}

/* LAMBDACHIP_ZEPHYR *************************************** LAMBDACHIP_LINUX */

#elif defined LAMBDACHIP_LINUX
static object_t _os_get_board_id (vm_t vm)
{
  // static char[] board_id = "GNU/Linux";
  // return board_id;
  // object_t obj;
  // Object obj = {.attr = {.type = mut_string, .gc = FREE_OBJ}, .value = 0};
  // obj = {.attr = {.type = } };
  // FIXME: return create a mut_string and return
  return (object_t)NULL;
}

static object_t _os_device_configure (vm_t vm, object_t ret, object_t dev)
{
  VALIDATE (dev, symbol);
  os_printk ("object_t _os_device_configure (%s)\n", (const char *)dev->value);
  *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_set (vm_t vm, object_t ret, object_t dev, object_t v)
{
  VALIDATE (dev, symbol);
  VALIDATE (v, boolean);

  os_printk ("object_t _os_gpio_set (%s, %d)\n", (const char *)dev->value,
             (imm_int_t)v->value);

  *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_toggle (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE (obj, symbol);

  os_printk ("object_t _os_gpio_toggle (%s)\n", (const char *)obj->value);
  *ret = GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_byte (vm_t vm, object_t ret, object_t dev,
                                   object_t dev_addr, object_t reg_addr)
{
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  os_printk ("i2c_reg_read_byte (%s, 0x%02X, 0x%02X)\n", (char *)dev->value,
             (imm_int_t)dev_addr->value, (imm_int_t)reg_addr->value);
  *ret = GLOBAL_REF (false_const);
  return ret;
}

static object_t _os_i2c_write_byte (vm_t vm, object_t ret, object_t dev,
                                    object_t dev_addr, object_t reg_addr,
                                    object_t value)
{
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  VALIDATE (value, imm_int);
  os_printk ("i2c_reg_write_byte (%s, 0x%02X, 0x%02X, 0x%02X)\n",
             (const char *)dev->value, (imm_int_t)dev_addr->value,
             (imm_int_t)reg_addr->value, (imm_int_t)value->value);
  *ret = GLOBAL_REF (false_const);
  return ret;
}

// TODO:
// FIXME: LambdaChip Linux _os_spi_transceive
static object_t _os_spi_transceive (vm_t vm, object_t ret, object_t dev,
                                    object_t spi_config, object_t send_buffer,
                                    object_t receive_buffer)
{
  VALIDATE (dev, symbol);
  VALIDATE (spi_config, list);
  VALIDATE (send_buffer, list);
  VALIDATE (receive_buffer, list);

  list_t send_buffer_raw = (list_t)send_buffer->value;
  list_t receive_buffer_raw = (list_t)receive_buffer->value;
  list_t spi_config_raw = (list_t)spi_config->value;

  // super_device *p = translate_supper_dev_from_symbol (dev);

  Object len; //  = CREATE_RET_OBJ ();
  object_t len_ptr = &len;
  len_ptr = _list_length (vm, len_ptr, send_buffer);

  object_printer (spi_config);
  object_printer (send_buffer);
  object_printer (receive_buffer);
  os_printk ("len_ptr = ");
  object_printer (len_ptr);
  os_printk ("\n");

  ListHead *send_buffer_head = LIST_OBJECT_HEAD (send_buffer);
  list_node_t send_buffer_node = SLIST_FIRST (send_buffer_head);

  u8_t *send_buffer_array = (u8_t *)GC_MALLOC ((imm_int_t) (len_ptr->value));
  if (!send_buffer_array)
    {
      *ret = GLOBAL_REF (false_const);
      return ret;
    }

  u32_t idx = 0;
  SLIST_FOREACH (send_buffer_node, send_buffer_head, next)
  {
    // VALIDATE (send_buffer_node->obj, imm_int);
    imm_int_t v = (uint8_t)send_buffer_node->obj;
    if (!(v < 256 && v >= 0))
      {
        *ret = GLOBAL_REF (false_const);
        return ret;
      }
    send_buffer_array[idx] = (u8_t)v;
    idx++;
  }

  int status = 0;
  if (status != 0)
    *ret = GLOBAL_REF (false_const);
  else
    *ret = GLOBAL_REF (none_const);
  return ret;
}

#  ifdef SIMULATE
struct super_device
{
  int dev;
};

int i2c_read (int dev, u8_t *rx_buf, int len_list, int addr)
{
  static u8_t data[] = {0x1C, 0x9A, 0x15, 0x34, 0xED, 0xAD, 0x00};
  memcpy (rx_buf, data, len_list);
  return 0;
}
#  endif /* SIMULATE*/

static object_t _os_i2c_read_list (vm_t vm, object_t ret, object_t dev,
                                   object_t i2c_addr, object_t length)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
#  ifndef SIMULATE
  os_printk ("i2c_reg_read_list (%s, 0x%02X, %d)\n", (const char *)dev->value,
             (imm_int_t)i2c_addr->value, (imm_int_t)length->value);
  *ret = GLOBAL_REF (false_const);
  return ret;
#  else
  struct super_device super_dev_0 = {0};
  struct super_device *p = &super_dev_0;

  imm_int_t len_list = (imm_int_t)length->value;
  uint8_t *rx_buf = (uint8_t *)GC_MALLOC (len_list);
  if (!rx_buf)
    {
      ret->attr.type = boolean;
      ret->value = (object_t)&GLOBAL_REF (false_const);
      PANIC ("Allocate buffer fail\n");
      return ret;
    }

  int status = i2c_read (p->dev, rx_buf, len_list, (imm_int_t)i2c_addr->value);
  if (status != 0)
    {
      // FIXME: ret is created outside, it may have memory leak here
      *ret = GLOBAL_REF (false_const);
      os_free (rx_buf);
      rx_buf = (void *)NULL;
      return ret;
    }

  list_t l = NEW_INNER_OBJ (list);
  SLIST_INIT (&l->list);
  l->attr.gc = PERMANENT_OBJ; // avoid unexpected collection by GC before done
  l->non_shared = 0;
  ret->attr.type = list;
  ret->attr.gc = PERMANENT_OBJ;
  ret->value = (void *)l;

  /* NOTE:
   * To safely created a List, we have to consider that GC may happend
   * unexpectedly.
   * 1. We must save list-obj to avoid to be freed by GC.
   * 2. The POP operation must be fixed to skip list-obj.
   * 3. oln must be created and inserted before the object allocation.
   */

  list_node_t iter = NULL;
  for (imm_int_t i = 0; i < len_list; i++)
    {
      list_node_t bl = NEW_LIST_NODE ();
      // avoid crash in case GC was triggered here
      bl->obj = (void *)0xDEADBEEF;
      object_t new_obj = NEW_OBJ (imm_int);
      new_obj->attr.gc = GEN_1_OBJ;
      bl->obj = new_obj;

      new_obj->value = (void *)rx_buf[i];
      if (0 == i)
        {
          SLIST_INSERT_HEAD (&l->list, bl, next);
        }
      else
        {
          SLIST_INSERT_AFTER (iter, bl, next);
        }
      iter = bl;
    }

  ret->attr.gc = GEN_1_OBJ;
  l->attr.gc = GEN_1_OBJ;
  os_free (rx_buf);
  rx_buf = (void *)NULL;
  return ret;
#  endif /* SIMULATE */
}

static object_t _os_i2c_read_bytevector (vm_t vm, object_t ret, object_t dev,
                                         object_t i2c_addr, object_t length)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
#  ifndef SIMULATE
  os_printk ("i2c_read_bytevector (%s, 0x%02X, %d)\n", (const char *)dev->value,
             (imm_int_t)i2c_addr->value, (imm_int_t)length->value);
  *ret = GLOBAL_REF (false_const);
  return ret;
#  else
  struct super_device super_dev_0 = {0};
  struct super_device *p = &super_dev_0;

  imm_int_t len_list = (imm_int_t)length->value;
  uint8_t *buf = (uint8_t *)GC_MALLOC (len_list);
  if (!buf)
    {
      ret->attr.type = boolean;
      ret->value = (object_t)&GLOBAL_REF (false_const);
      PANIC ("GC_MALLOC fail\n");
      return ret;
    }

  int status = i2c_read (p->dev, buf, len_list, (imm_int_t)i2c_addr->value);
  if (status != 0)
    {
      // FIXME: ret is created outside, it may have memory leak here
      *ret = GLOBAL_REF (false_const);
      // ret->attr.type = boolean;
      // ret->value = (object_t)&GLOBAL_REF (false_const);
      os_free (buf);
      buf = (void *)NULL;
      return ret;
    }

  bytevector_t bv = NEW_INNER_OBJ (bytevector);
  bv->attr.gc = PERMANENT_OBJ; // avoid unexpected collection by GC before done
  bv->vec = buf;
  bv->size = len_list;
  ret->attr.type = bytevector;
  ret->attr.gc = PERMANENT_OBJ;
  ret->value = (void *)bv;

  ret->attr.gc = GEN_1_OBJ;
  bv->attr.gc = GEN_1_OBJ;
  // the ownership of buf is transfered to ret->bv->vec
  // do not need to free here
  return ret;
#  endif /* SIMULATE */
}

static object_t _os_i2c_write_list (vm_t vm, object_t ret, object_t dev,
                                    object_t i2c_addr, object_t lst)
{
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (lst, list);
  os_printk ("i2c_reg_write_list (%s, 0x%02X, ", (const char *)dev->value,
             (imm_int_t)i2c_addr->value);
  object_printer (lst);
  os_printk (")\n");
  *ret = GLOBAL_REF (false_const);
  return ret;
}

#endif /* LAMBDACHIP_LINUX */

static Object prim_procedure_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, procedure);
}

static Object prim_primitive_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, primitive);
}

static Object prim_boolean_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, boolean);
}

static Object prim_number_p (object_t obj)
{
  bool ret = false;

  switch (obj->attr.type)
    {
    case imm_int:
    case real:
    case rational_pos:
    case rational_neg:
    case complex_exact:
    case complex_inexact:
      {
        ret = true;
        break;
      }
    default:
      {
        ret = false;
      }
    }

  return (ret ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const));
}

static Object prim_integer_p (object_t obj)
{
  bool ret = false;

  switch (obj->attr.type)
    {
    case imm_int:
      {
        ret = true;
        break;
      }
    case real:
      {
        s32_t v = (s32_t)obj->value;
        real_t r = {0};
        r.v = (uintptr_t)obj->value;
        ret = (v == r.f);
        break;
      }
    default:
      {
        ret = false;
      }
    }

  return (ret ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const));
}

static Object prim_real_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, real);
}

static Object prim_rational_p (object_t obj)
{
  switch (obj->attr.type)
    {
    case rational_pos:
    case rational_neg:
      {
        return GLOBAL_REF (true_const);
      }
    default:
      {
        return GLOBAL_REF (false_const);
      }
    }
}

static Object prim_complex_p (object_t obj)
{
  switch (obj->attr.type)
    {
    case complex_exact:
    case complex_inexact:
      {
        return GLOBAL_REF (true_const);
      }
    default:
      {
        return GLOBAL_REF (false_const);
      }
    }
}

static Object prim_bytevector_p (object_t obj)
{
  switch (obj->attr.type)
    {
    case bytevector:
    case mut_bytevector:
      {
        return GLOBAL_REF (true_const);
      }
    default:
      {
        return GLOBAL_REF (false_const);
      }
    }
}

void primitives_init (void)
{
  /* NOTE: If fn is NULL, then it's inlined to call_prim
   */
  def_prim (0, "ret", 0, NULL);
  def_prim (1, "pop", 0, NULL);
  def_prim (2, "add", 2, (void *)_int_add);
  def_prim (3, "sub", 2, (void *)_int_sub);
  def_prim (4, "mul", 2, (void *)_int_mul);
  def_prim (5, "div", 2, (void *)_int_div);
  def_prim (6, "object_print", 1, (void *)_object_print);
  def_prim (7, "apply", 2, NULL);
  def_prim (8, "not", 1, (void *)_not);
  def_prim (9, "int_eq", 2, (void *)_int_eq);
  def_prim (10, "int_lt", 2, (void *)_int_lt);
  def_prim (11, "int_gt", 2, (void *)_int_gt);
  def_prim (12, "int_le", 2, (void *)_int_le);
  def_prim (13, "int_ge", 2, (void *)_int_ge);
  def_prim (14, "restore", 0, NULL);
  def_prim (15, "reserved_1", 0, NULL);

  // extended primitives
  def_prim (16, "modulo", 2, (void *)_int_modulo);
  def_prim (17, "remainder", 2, (void *)_int_remainder);
  def_prim (18, "foreach", 2, NULL);
  def_prim (19, "map", 2, NULL);
  def_prim (20, "list_ref", 2, (void *)_list_ref);
  def_prim (21, "list_set", 3, (void *)_list_set);
  def_prim (22, "list_append", 2, (void *)_list_append);
  def_prim (23, "eq", 2, (void *)_eq);
  def_prim (24, "eqv", 2, (void *)_eqv);
  def_prim (25, "equal", 2, (void *)_equal);
  def_prim (26, "prim_usleep", 1, (void *)_os_usleep);
  def_prim (27, "device_configure", 2, (void *)_os_device_configure);
  def_prim (28, "gpio_set", 2, (void *)_os_gpio_set);
  def_prim (29, "gpio_toggle", 1, (void *)_os_gpio_toggle);
  def_prim (30, "get_board_id", 2, (void *)_os_get_board_id);
  def_prim (31, "cons", 2, (void *)_cons);
  def_prim (32, "car", 1, (void *)_car);
  def_prim (33, "cdr", 1, (void *)_cdr);
  def_prim (34, "read_char", 0, (void *)_read_char);
  def_prim (35, "read_str", 1, (void *)_read_str);
  def_prim (36, "read_line", 0, (void *)_read_line);
  def_prim (37, "list_to_string", 1, (void *)_list_to_string);
  def_prim (38, "i2c_read_byte", 3, (void *)_os_i2c_read_byte);
  def_prim (39, "i2c_write_byte", 4, (void *)_os_i2c_write_byte);
  def_prim (40, "null?", 1, (void *)prim_null_p);
  def_prim (41, "pair?", 1, (void *)prim_pair_p);
  def_prim (42, "spi_transceive", 4, (void *)_os_spi_transceive);
  def_prim (43, "i2c_read_list", 3, (void *)_os_i2c_read_list);
  def_prim (44, "i2c_write_list", 3, (void *)_os_i2c_write_list);
  def_prim (45, "with-exception-handler", 2, NULL);
  def_prim (46, "scm_raise", 1, NULL);
  def_prim (47, "scm_raise_continuable", 1, NULL);
  def_prim (48, "scm_error", 1, NULL);
  def_prim (49, "error-object?", 1, NULL);
  def_prim (50, "error-object-message", 1, NULL);
  def_prim (51, "error-object-irritants", 1, NULL);
  def_prim (52, "read-error", 1, NULL);
  def_prim (53, "file-error?", 1, NULL);
  def_prim (54, "dynamic-wind", 3, NULL);
  def_prim (55, "list?", 1, prim_list_p);
  def_prim (56, "string?", 1, prim_string_p);
  def_prim (57, "char?", 1, prim_char_p);
  def_prim (58, "keyword?", 1, prim_keyword_p);
  def_prim (59, "symbol?", 1, prim_symbol_p);
  def_prim (60, "procedure?", 1, prim_procedure_p);
  def_prim (61, "primitive?", 1, prim_primitive_p);
  def_prim (62, "boolean?", 1, prim_boolean_p);
  def_prim (63, "number?", 1, prim_number_p);
  def_prim (64, "integer?", 1, prim_integer_p);
  def_prim (65, "real?", 1, prim_real_p);
  def_prim (66, "rational?", 1, prim_rational_p);
  def_prim (67, "complex?", 1, prim_complex_p);
  def_prim (68, "exact?", 1, prim_complex_p);
  def_prim (69, "inexact?", 1, prim_complex_p);
  def_prim (70, "i2c-read-bytevector!", 3, (void *)_os_i2c_read_bytevector);
  def_prim (71, "bytevector?", 1, prim_bytevector_p);
}

char *prim_name (u16_t pn)
{
#if defined LAMBDACHIP_DEBUG
  if (pn >= PRIM_MAX)
    {
      PANIC ("Invalid prim number: %d\nprim_name halt\n", pn);
    }

  return GLOBAL_REF (prim_table)[pn]->name;
#else
  return "";
#endif
}

prim_t get_prim (u16_t pn)
{
  return GLOBAL_REF (prim_table)[pn];
}

void primitives_clean (void)
{
  for (int i = 0; i < PRIM_MAX; i++)
    {
      void *p = GLOBAL_REF (prim_table)[i];
      if (p)
        os_free (p);
      GLOBAL_REF (prim_table)[i] = NULL;
    }
}
