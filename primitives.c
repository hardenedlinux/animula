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

GLOBAL_DEF (prim_t, prim_table[PRIM_MAX]) = {0};

// primitives implementation

static inline object_t _int_add (vm_t vm, object_t ret, object_t x, object_t y)
{
  if (x->attr.type == complex_inexact || y->attr.type == complex_inexact)
    {
    }
  else if (x->attr.type == complex_exact || y->attr.type == complex_exact)
    {
    }
  else if (x->attr.type == real || y->attr.type == real)
    {
#ifdef LAMBDACHIP_LITTLE_ENDIAN
      real_t a;
      real_t b;
      if (x->attr.type == real)
        {
          memcpy (&(a), &(x->value), 4);
        }
      else if ((x->attr.type == rational_pos) || (x->attr.type == rational_neg))
        {
          int sign = (x->attr.type == rational_pos) ? 1 : -1;
          // FIXME: may lose precision
          a.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if (x->attr.type == imm_int)
        {
          // FIXME: may lose precision
          a.f = (float)(imm_int_t) (x->value);
        }
      else
        {
          os_printk ("%s:%d, %s: Invalid type, expect %d or %d, but it's %d\n",
                     __FILE__, __LINE__, __FUNCTION__, imm_int, real,
                     x->attr.type);
          panic ("");
        }

      if (y->attr.type == real)
        {
          memcpy (&(b), &(y->value), 4);
        }
      else if ((y->attr.type == rational_pos) || (y->attr.type == rational_neg))
        {
          int sign = (y->attr.type == rational_pos) ? 1 : -1;
          b.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if ((y->attr.type == imm_int))
        {
          b.f = (float)(imm_int_t) (y->value);
        }
      else
        {
          os_printk ("%s:%d, %s: Invalid type, expect %d or %d, but it's %d\n",
                     __FILE__, __LINE__, __FUNCTION__, imm_int, real,
                     y->attr.type);
          panic ("");
        }
      float c = a.f + b.f;
      ret->attr.type = real;
      memcpy (&(ret->value), &c, 4);
#else
#  error "BIG_ENDIAN not provided"
#endif
    }
  else if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos)
           || (y->attr.type == rational_neg) || (y->attr.type == rational_pos))
    {
      if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos))
        {
          if ((y->attr.type == rational_neg) || (y->attr.type == rational_pos))
            {
            }
          else if (y->attr.type == imm_int)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (y);
            }
          else
            {
              os_printk ("%s:%d, %s: Type error, %d\n", __FILE__, __LINE__,
                         __FUNCTION__, y->attr.type);
              panic ("");
            }
        }
      if ((y->attr.type == rational_neg) || (y->attr.type == rational_pos))
        {
          if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos))
            {
            }
          else if (x->attr.type == imm_int)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (x);
            }
          else
            {
              os_printk ("%s:%d, %s: Type error, %d\n", __FILE__, __LINE__,
                         __FUNCTION__, x->attr.type);
              panic ("");
            }
        }
      // TODO: check if s32 enough
      s64_t xd, xn, yd, yn, x_sign, y_sign;
      s64_t denominator, numerator, common_divisor;
      xn = ((imm_int_t) (x->value) >> 16) & 0xFFFF;
      xd = ((imm_int_t) (x->value) & 0xFFFF);
      x_sign = (x->attr.type == rational_pos) ? 1 : -1;
      yn = ((imm_int_t) (y->value) >> 16) & 0xFFFF;
      yd = ((imm_int_t) (y->value) & 0xFFFF);
      y_sign = (y->attr.type == rational_pos) ? 1 : -1;

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
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          float a;
          float b;
          memcpy (&a, &(x->value), 4);
          memcpy (&b, &(y->value), 4);
          b = a + b;
          ret->attr.type = real;
          memcpy (&(ret->value), &b, 4);

#else
#  error "BIG_ENDIAN not provided"
#endif
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
          // convert to float
          cast_rational_to_float (x);
          cast_rational_to_float (y);
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          float a;
          float b;
          memcpy (&a, &(x->value), 4);
          memcpy (&b, &(y->value), 4);
          b = a / b;
          ret->attr.type = real;
          memcpy (&(ret->value), &b, 4);

#else
#  error "BIG_ENDIAN not provided"
#endif
        }

      // ret->attr.type == (numerator>0)?rational_pos:rational_neg;
      // ret->value == ()
      return ret;
    }
  else if (x->attr.type == imm_int && y->attr.type == imm_int)
    {
      s64_t result = ((s64_t)x->value + (s64_t)y->value);
      s32_t result2 = 0xFFFFFFFF & result;
      float result3;
      if (result2 != result) // if overflow
        {
          // os_printk ("%s:%d, %s: Add overflow or underflow %lld, %d\n",
          //            __FILE__, __LINE__, __FUNCTION__, result, result2);
          // panic ("");
          result3 = (float)result;
#ifdef LAMBDACHIP_LITTLE_ENDIAN

          memcpy (&(ret->value), &result3, 4);
#else
#  error "BIG_ENDIAN not provided"
#endif
          // ret->value = (void *)result2;
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
      os_printk (
        "%s:%d, %s: Type error, x->attr.type == %d && y->attr.type == %d\n",
        __FILE__, __LINE__, __FUNCTION__, x->attr.type, y->attr.type);
      panic ("");
      return ret;
    }

  return ret;
}

static inline object_t _int_sub (vm_t vm, object_t ret, object_t x, object_t y)
{
  if (y->attr.type == real)
    {
#ifdef LAMBDACHIP_LITTLE_ENDIAN
      float a;
      memcpy (&a, &(y->value), 4);
      a = -a;
      memcpy (&(y->value), &a, 4);
#else
#  error "BIG_ENDIAN not provided"
#endif
    }
  else if (y->attr.type == rational_pos)
    {
      y->attr.type = rational_neg;
    }
  else if (y->attr.type == rational_neg)
    {
      y->attr.type = rational_pos;
    }
  else if (y->attr.type == imm_int)
    {
      // FIXME:side effect
      if ((imm_int_t) (y->value) == MIN_INT32) // -1*2^31
        {
          os_printk (
            "%s:%d, %s: Out of range, cannot calculate opposite number of %d\n",
            __FILE__, __LINE__, __FUNCTION__, (imm_int_t) (y->value));
          panic ("");
        }
      else
        {
          y->value = (void *)(((imm_int_t) (y->value)) * -1);
        }
    }
  else
    {
      os_printk (
        "%s:%d, %s: Invalid type, expect %d, %d, %d or %d, but it's %d\n",
        __FILE__, __LINE__, __FUNCTION__, real, rational_pos, rational_neg,
        imm_int, y->attr.type);
      panic ("");
    }
  return _int_add (vm, ret, x, y);
}

static inline object_t _int_mul (vm_t vm, object_t ret, object_t x, object_t y)
{
  if (x->attr.type == complex_inexact || y->attr.type == complex_inexact)
    {
    }
  else if (x->attr.type == complex_exact || y->attr.type == complex_exact)
    {
    }
  if (x->attr.type == complex_inexact || y->attr.type == complex_inexact)
    {
    }
  else if (x->attr.type == complex_exact || y->attr.type == complex_exact)
    {
    }
  else if (x->attr.type == real || y->attr.type == real)
    {
#ifdef LAMBDACHIP_LITTLE_ENDIAN
      real_t a;
      real_t b;
      if (x->attr.type == real)
        {
          memcpy (&(a), &(x->value), 4);
        }
      else if ((x->attr.type == rational_pos) || (x->attr.type == rational_neg))
        {
          int sign = (x->attr.type == rational_pos) ? 1 : -1;
          // FIXME: may lose precision
          a.f = sign * (((imm_int_t) (x->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (x->value) & 0xFFFF);
        }
      else if (x->attr.type == imm_int)
        {
          // FIXME: may lose precision
          a.f = (float)(imm_int_t) (x->value);
        }
      else
        {
          os_printk ("%s:%d, %s: Invalid type, expect %d or %d, but it's %d\n",
                     __FILE__, __LINE__, __FUNCTION__, imm_int, real,
                     x->attr.type);
          panic ("");
        }

      if (y->attr.type == real)
        {
          memcpy (&(b), &(y->value), 4);
        }
      else if ((y->attr.type == rational_pos) || (y->attr.type == rational_neg))
        {
          int sign = (y->attr.type == rational_pos) ? 1 : -1;
          b.f = sign * (float)(((imm_int_t) (y->value) >> 16) & 0xFFFF)
                / (float)((imm_int_t) (y->value) & 0xFFFF);
        }
      else if ((y->attr.type == imm_int))
        {
          // FIXME: may lose precision
          b.f = (float)(imm_int_t) (y->value);
        }
      else
        {
          os_printk ("%s:%d, %s: Invalid type, expect %d or %d, but it's %d\n",
                     __FILE__, __LINE__, __FUNCTION__, imm_int, real,
                     y->attr.type);
          panic ("");
        }
      float c = a.f * b.f;
      ret->attr.type = real;
      memcpy (&(ret->value), &c, 4);
#else
#  error "BIG_ENDIAN not provided"
#endif
    }
  else if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos)
           || (y->attr.type == rational_neg) || (y->attr.type == rational_pos))
    {
      if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos))
        {
          if ((y->attr.type == rational_neg) || (y->attr.type == rational_pos))
            {
            }
          else if (y->attr.type == imm_int)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (y);
            }
          else
            {
              os_printk ("%s:%d, %s: Type error, %d\n", __FILE__, __LINE__,
                         __FUNCTION__, y->attr.type);
              panic ("");
            }
        }
      if ((y->attr.type == rational_neg) || (y->attr.type == rational_pos))
        {
          if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos))
            {
            }
          else if (x->attr.type == imm_int)
            {
              // side effect
              // FIXME: if integer canont convert to rational
              cast_imm_int_to_rational (x);
            }
          else
            {
              os_printk ("%s:%d, %s: Type error, %d\n", __FILE__, __LINE__,
                         __FUNCTION__, x->attr.type);
              panic ("");
            }
        }
      // TODO: check if s32 enough
      s64_t xd, xn, yd, yn, x_sign, y_sign;
      s64_t denominator, numerator, common_divisor;
      xn = ((imm_int_t) (x->value) >> 16) & 0xFFFF;
      xd = ((imm_int_t) (x->value) & 0xFFFF);
      x_sign = (x->attr.type == rational_pos) ? 1 : -1;
      yn = ((imm_int_t) (y->value) >> 16) & 0xFFFF;
      yd = ((imm_int_t) (y->value) & 0xFFFF);
      // FIXME: if integer canont convert to rational
      y_sign = (y->attr.type == rational_pos) ? 1 : -1;

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
      else
        {
          // side effect
          cast_rational_to_float (x);
          // side effect
          cast_rational_to_float (y);
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          float a;
          float b;
          memcpy (&a, &(x->value), 4);
          memcpy (&b, &(y->value), 4);
          b = a * b;
          ret->attr.type = real;
          memcpy (&(ret->value), &b, 4);
          return ret;
#else
#  error "BIG_ENDIAN not provided"
#endif
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
      else
        {
          // convert to float
          cast_rational_to_float (x);
          cast_rational_to_float (y);
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          float a;
          float b;
          memcpy (&a, &(x->value), 4);
          memcpy (&b, &(y->value), 4);
          b = a * b;
          ret->attr.type = real;
          memcpy (&(ret->value), &b, 4);

#else
#  error "BIG_ENDIAN not provided"
#endif
        }

      // ret->attr.type == (numerator>0)?rational_pos:rational_neg;
      // ret->value == ()
      return ret;
    }
  else if (x->attr.type == imm_int && y->attr.type == imm_int)
    {
      s64_t result = ((s64_t)x->value * (s64_t)y->value);
      s32_t result2 = 0xFFFFFFFF & result;
      float result3;
      if (result2 != result) // if overflow
        {
          // os_printk ("%s:%d, %s: Add overflow or underflow %lld, %d\n",
          //            __FILE__, __LINE__, __FUNCTION__, result, result2);
          // panic ("");
          result3 = (float)result;
#ifdef LAMBDACHIP_LITTLE_ENDIAN

          memcpy (&(ret->value), &result3, 4);
#else
#  error "BIG_ENDIAN not provided"
#endif
          // ret->value = (void *)result2;
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
      os_printk (
        "%s:%d, %s: Type error, x->attr.type == %d && y->attr.type == %d\n",
        __FILE__, __LINE__, __FUNCTION__, x->attr.type, y->attr.type);
      panic ("");
      return ret;
    }

  return ret;
}

static inline object_t _int_div (vm_t vm, object_t ret, object_t x, object_t y)
{
  if (x->attr.type == complex_inexact || y->attr.type == complex_inexact)
    {
    }
  else if (x->attr.type == complex_exact || y->attr.type == complex_exact)
    {
    }
  else if (x->attr.type == real || y->attr.type == real)
    {
#ifndef LAMBDACHIP_LITTLE_ENDIAN
#  error "BIG_ENDIAN not provided"
#endif
      float a;
      float b;
      cast_int_or_fractal_to_float (x);
      cast_int_or_fractal_to_float (y);
      memcpy (&a, &(x->value), sizeof (a));
      memcpy (&b, &(y->value), sizeof (b));
      if (b != 0.0f)
        {
          b = a / b;
        }
      else
        {
          os_printk ("%s:%d, %s: Div by 0 error!\n", __FILE__, __LINE__,
                     __FUNCTION__);
          panic ("");
        }
      memcpy (&(ret->value), &b, sizeof (b));
      ret->attr.type = real;
    }
  else if ((x->attr.type == rational_neg) || (x->attr.type == rational_pos)
           || (y->attr.type == rational_neg) || (y->attr.type == rational_pos))
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

      if ((x->attr.type == rational_pos) || (x->attr.type == rational_neg))
        {
          nx = (((imm_int_t) (x->value)) >> 16) & 0xFFFF;
          dx = ((imm_int_t) (x->value)) & 0xFFFF;
          sign_x = (x->attr.type == rational_pos) ? 1 : -1;
        }
      else if (x->attr.type == imm_int)
        {
          nx = (imm_int_t) (x->value);
          dx = 1;
          sign_x = (nx >= 0) ? 1 : -1;
          nx = abs (nx);
        }
      else
        {
          os_printk (
            "%s:%d, %s: Invalid type, expect %d, %d or %d, but it's %d\n",
            __FILE__, __LINE__, __FUNCTION__, rational_pos, rational_neg,
            imm_int, x->attr.type);
          panic ("");
        }
      if ((y->attr.type == rational_pos) || (y->attr.type == rational_neg))
        {
          ny = (((imm_int_t) (y->value)) >> 16) & 0xFFFF;
          dy = ((imm_int_t) (y->value)) & 0xFFFF;
          sign_y = (y->attr.type == rational_pos) ? 1 : -1;
        }
      else if (y->attr.type == imm_int)
        {
          ny = (imm_int_t) (y->value);
          dy = 1;
          sign_y = (ny >= 0) ? 1 : -1;
          ny = abs (ny);
        }
      else
        {
          os_printk (
            "%s:%d, %s: Invalid type, expect %d, %d or %d, but it's %d\n",
            __FILE__, __LINE__, __FUNCTION__, rational_pos, rational_neg,
            imm_int, x->attr.type);
          panic ("");
        }
      nn = nx * dy;
      dd = dx * ny;
      nn = abs64 (nn);
      dd = abs64 (dd);
      sign = sign_x * sign_y;
      common_divisor64 = gcd64 (nn, dd);
      nn = nn / common_divisor64;
      dd = dd / common_divisor64;
      if (dd == 0)
        {
          os_printk ("%s:%d, %s: Div by 0 error!\n", __FILE__, __LINE__,
                     __FUNCTION__);
          panic ("");
        }
      if (nn < MIN_REAL_DENOMINATOR || nn > MAX_REAL_DENOMINATOR
          || dd < MIN_REAL_DENOMINATOR || dd > MAX_REAL_DENOMINATOR)
        {
          float a = (float)(nn * sign) / (float)dd;
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          memcpy (&(ret->value), &a, sizeof (a));
          ret->attr.type = real;
#else
#  error "BIG_ENDIAN not provided"
#endif
        }
      else // nn and dd are in range
        {
          ret->value = (void *)((nn << 16) | dd);
          ret->attr.type = (sign >= 0) ? rational_pos : rational_neg;
          cast_rational_to_imm_int_if_denominator_is_1 (ret);
        }
    }
  else if (x->attr.type == imm_int && y->attr.type == imm_int)
    {
      imm_int_t n = (imm_int_t) (x->value);
      imm_int_t d = (imm_int_t) (y->value);
      imm_int_t sign = (n >= 0) ? 1 : -1;
      sign = sign * ((d >= 0) ? 1 : -1);
      n = abs (n);
      d = abs (d);

      if (d == 0)
        {
          os_printk ("%s:%d, %s: Div by 0 error!\n", __FILE__, __LINE__,
                     __FUNCTION__);
          panic ("");
        }
      imm_int_t common_divisor = gcd (n, d);
      n = n / common_divisor;
      d = d / common_divisor;

      if (n < MIN_REAL_DENOMINATOR || n > MAX_REAL_DENOMINATOR
          || d < MIN_REAL_DENOMINATOR || d > MAX_REAL_DENOMINATOR)
        {
          ret->attr.type = real;
          float c = sign * (float)n / (float)d;
#ifdef LAMBDACHIP_LITTLE_ENDIAN
          memcpy (&(ret->value), &c, sizeof (c));
#else
#  error "BIG_ENDIAN not provided"
#endif
          // return ret;
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
      os_printk (
        "%s:%d, %s: Type error, x->attr.type == %d && y->attr.type == %d\n",
        __FILE__, __LINE__, __FUNCTION__, x->attr.type, y->attr.type);
      panic ("");
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

static bool _int_eq (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  // os_printk ("%p == %p is %d\n", x->value, y->value, x->value == y->value);
  return (imm_int_t)x->value == (imm_int_t)y->value;
}

static bool _int_lt (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value < (imm_int_t)y->value;
}

static bool _int_gt (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value > (imm_int_t)y->value;
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
      /* case float: */
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
        os_printk ("eqv?: The type %d hasn't implemented yet\n", t1);
        panic ("");
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
            obj_list_head_t *h1 = LIST_OBJECT_HEAD (a);
            obj_list_head_t *h2 = LIST_OBJECT_HEAD (b);
            obj_list_t n1 = NULL;
            obj_list_t n2 = SLIST_FIRST (h2);

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
        panic ("equal?: vector hasn't been implemented yet!\n");
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
  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t prim_is_null (vm_t vm, object_t ret, object_t l)
{
  if (list == l->attr.type)
    {
      list_t lst = (list_t)l->value;
      obj_list_head_t *head = LIST_OBJECT_HEAD (l);
      obj_list_t node = SLIST_FIRST (head);
      if (node)
        {
          ret = &GLOBAL_REF (false_const);
        }
      else
        {
          ret = &GLOBAL_REF (true_const);
        }
    }
  else if (null_obj == l->attr.type)
    {
      ret = &GLOBAL_REF (true_const);
    }
  else
    {
      ret = &GLOBAL_REF (false_const);
    }
  return ret;
}

static object_t prim_is_pair (vm_t vm, object_t ret, object_t l)
{
  if (list == l->attr.type)
    {
      // if (eq l '()) return #f
      list_t lst = (list_t)l->value;
      obj_list_head_t *head = LIST_OBJECT_HEAD (l);
      obj_list_t node = SLIST_FIRST (head);
      if (node)
        {
          ret = &GLOBAL_REF (true_const);
        }
      else
        {
          ret = &GLOBAL_REF (false_const);
        }
    }
  else if (pair == l->attr.type)
    {
      ret = &GLOBAL_REF (true_const);
    }
  else
    {
      ret = &GLOBAL_REF (false_const);
    }
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
  static const char char_gpio_pa9[] = "dev_gpio_pa9";
  static const char char_gpio_pa10[] = "dev_gpio_pa10";
  static const char char_gpio_pb4[] = "dev_gpio_pb4";
  static const char char_gpio_pa8[] = "dev_gpio_pa8";
  static const char char_gpio_pb5[] = "dev_gpio_pb5";
  static const char char_gpio_pb6[] = "dev_gpio_pb6";
  static const char char_gpio_pb7[] = "dev_gpio_pb7";
  static const char char_gpio_pb8[] = "dev_gpio_pb8";
  static const char char_gpio_pb9[] = "dev_gpio_pb9";
  static const char char_gpio_pa2[] = "dev_gpio_pa2";
  static const char char_gpio_pa3[] = "dev_gpio_pa3";
  static const char char_gpio_pb3[] = "dev_gpio_pb3";
  static const char char_gpio_pb10[] = "dev_gpio_pb10";
  static const char char_gpio_pb15[] = "dev_gpio_pb15";
  static const char char_gpio_pb14[] = "dev_gpio_pb14";
  static const char char_gpio_pb13[] = "dev_gpio_pb13";
  static const char char_gpio_pb12[] = "dev_gpio_pb12";
  static const char char_gpio_ble_disable[] = "dev_gpio_ble_disable";
  static const char char_i2c2[] = "dev_i2c2";
  static const char char_i2c3[] = "dev_i2c3";

  const char *str_buf = (const char *)sym->value;
  size_t len = os_strnlen (str_buf, MAX_STR_LEN);
  if (0 == os_strncmp (str_buf, char_dev_led0, len))
    {
      ret = &(GLOBAL_REF (super_dev_led0));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led1, len))
    {
      ret = &(GLOBAL_REF (super_dev_led1));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led2, len))
    {
      ret = &(GLOBAL_REF (super_dev_led2));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led3, len))
    {
      ret = &(GLOBAL_REF (super_dev_led3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa9, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa9));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa10, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa10));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb4, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb4));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa8, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa8));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb5, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb5));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb6, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb6));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb7, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb7));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb8, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb8));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb9, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb9));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa2, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa2));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa3, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb3, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb10, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb10));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb15, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb15));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb14, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb14));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb13, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb13));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb12, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb12));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_ble_disable, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_ble_disable));
    }
  else if (0 == os_strncmp (str_buf, char_i2c2, len))
    {
      ret = &(GLOBAL_REF (super_dev_i2c2));
    }
  else if (0 == os_strncmp (str_buf, char_i2c3, len))
    {
      ret = &(GLOBAL_REF (super_dev_i2c3));
    }
  else
    {
      os_printk ("BUG: Invalid symbol name %s!\n", str_buf);
      panic ("");
    }

  return ret;
}

static object_t _os_device_configure (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE (obj, symbol);

  ret = &GLOBAL_REF (none_const);
  // const char *str_buf = GET_SYBOL ((u32_t)obj->value);
  super_device *p = translate_supper_dev_from_symbol (obj);

  // FIXME: flags for different pin shall be different
  if (p->type == SUPERDEVICE_TYPE_GPIO_PIN)
    {
      gpio_pin_configure (p->dev, p->gpio_pin, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    }
  else if (p->type == SUPERDEVICE_TYPE_I2C)
    {
      i2c_configure (p->dev, 0);
    }
  else if (p->type == SUPERDEVICE_TYPE_SPI)
    {
    }
  else
    {
      os_printk ("%s:%d, %s: device type not defined, cannot handle", __FILE__,
                 __LINE__, __FUNCTION__);
      panic ("");
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
  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_toggle (vm_t vm, object_t ret, object_t dev)
{
  VALIDATE (dev, symbol);
  super_device *p = translate_supper_dev_from_symbol (dev);
  gpio_pin_toggle (p->dev, p->gpio_pin);
  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_byte (vm_t vm, object_t ret, object_t dev,
                                   object_t dev_addr, object_t reg_addr)
{
  VALIDATE (ret, imm_int);
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
      ret = &GLOBAL_REF (false_const);
    }
  return ret;
}

static object_t _os_i2c_write_byte (vm_t vm, object_t ret, object_t dev,
                                    object_t dev_addr, object_t reg_addr,
                                    object_t value)
{
  VALIDATE (ret, imm_int);
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
    ret = &GLOBAL_REF (false_const);
  else
    ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_list (vm_t vm, object_t ret, object_t dev,
                                   object_t i2c_addr, object_t length)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
  super_device *p = translate_supper_dev_from_symbol (dev);

  imm_int_t len_list = (imm_int_t)length->value;
  uint8_t *rx_buf = (uint8_t *)os_malloc (len_list);
  if (!rx_buf)
    {
      ret->attr.type = boolean;
      ret->value = &GLOBAL_REF (false_const);
      os_printk ("%s, %s: malloc error\n", __FILE__, __FUNCTION__);
      return ret;
    }

  int status = i2c_read (p->dev, rx_buf, len_list, (imm_int_t)i2c_addr->value);
  if (status != 0)
    {
      ret = &GLOBAL_REF (false_const);
      free (rx_buf);
      rx_buf = (void *)NULL;
      return ret;
    }

  list_t l = NEW (list);
  SLIST_INIT (&l->list);
  ret->attr.type = list;
  ret->attr.gc = 1;
  ret->value = (void *)l;

  obj_list_t iter;
  for (imm_int_t i = 0; i < len_list; i++)
    {
      object_t new_obj = NEW_OBJ (0);

      // FIXME: check if pop is needed?
      // *new_obj = (object_t)POP_OBJ ();
      obj_list_t bl = NEW_OBJ_LIST_NODE ();
      bl->obj = new_obj;
      bl->obj->value = (void *)rx_buf[i];
      if (i == 0)
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
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (lst, list);
  super_device *p = translate_supper_dev_from_symbol (dev);

  Object len;
  object_t len_p = &len;
  // side effect
  len_p = _list_length (vm, len_p, lst);
  imm_int_t len_list = (imm_int_t)len_p->value;

  uint8_t *tx_buf = (uint8_t *)os_malloc (len_list);
  if (!tx_buf)
    {
      ret->attr.type = boolean;
      ret->value = &GLOBAL_REF (false_const);
      // os_printk("%s, _os_i2c_write_list(): malloc error\n");
      os_printk ("%s, %s: malloc error\n", __FILE__, __FUNCTION__);
      panic ("");
      return ret;
    }

  list_t obj_lst = (list_t) (lst->value);
  obj_list_head_t head = obj_lst->list;
  obj_list_t iter = {0};
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
    ret = &GLOBAL_REF (false_const);
  else
    ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_spi_transceive (vm_t vm, object_t ret, object_t dev,
                                    object_t spi_config, object_t send_buffer,
                                    object_t receive_buffer)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (spi_config, list);
  VALIDATE (send_buffer, list);
  VALIDATE (receive_buffer, list);
  // super_device *p = translate_supper_dev_from_symbol (dev);

  object_printer (spi_config);
  int status = 0;
  if (status != 0)
    ret = &GLOBAL_REF (false_const);
  else
    ret = &GLOBAL_REF (none_const);
  return ret;
}

/* LAMBDACHIP_ZEPHYR */
#elif defined LAMBDACHIP_LINUX
static object_t _os_get_board_id (vm_t vm)
{
  // static char[] board_id = "GNU/Linux";
  // return board_id;
  // object_t obj;
  // Object obj = {.attr = {.type = mut_string, .gc = 0}, .value = 0};
  // obj = {.attr = {.type = } };
  // FIXME: return create a mut_string and return
  return (object_t)NULL;
}

static object_t _os_device_configure (vm_t vm, object_t ret, object_t dev)
{
  VALIDATE (dev, symbol);
  os_printk ("object_t _os_device_configure (%s)\n", (const char *)dev->value);
  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_set (vm_t vm, object_t ret, object_t dev, object_t v)
{
  VALIDATE (dev, symbol);
  VALIDATE (v, boolean);

  os_printk ("object_t _os_gpio_set (%s, %d)\n", (const char *)dev->value,
             (imm_int_t)v->value);

  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_gpio_toggle (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE (obj, symbol);

  os_printk ("object_t _os_gpio_toggle (%s)\n", (const char *)obj->value);
  ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_byte (vm_t vm, object_t ret, object_t dev,
                                   object_t dev_addr, object_t reg_addr)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  os_printk ("i2c_reg_read_byte (%s, 0x%02X, 0x%02X)\n", (char *)dev->value,
             (imm_int_t)dev_addr->value, (imm_int_t)reg_addr->value);
  ret = &GLOBAL_REF (false_const);
  return ret;
}

static object_t _os_i2c_write_byte (vm_t vm, object_t ret, object_t dev,
                                    object_t dev_addr, object_t reg_addr,
                                    object_t value)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (dev_addr, imm_int);
  VALIDATE (reg_addr, imm_int);
  VALIDATE (value, imm_int);
  os_printk ("i2c_reg_write_byte (%s, 0x%02X, 0x%02X, 0x%02X)\n",
             (const char *)dev->value, (imm_int_t)dev_addr->value,
             (imm_int_t)reg_addr->value, (imm_int_t)value->value);
  ret = &GLOBAL_REF (false_const);
  return ret;
}

// TODO:
// FIXME: LambdaChip Linux _os_spi_transceive
static object_t _os_spi_transceive (vm_t vm, object_t ret, object_t dev,
                                    object_t spi_config, object_t send_buffer,
                                    object_t receive_buffer)
{
  VALIDATE (ret, imm_int);
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

  // typedef SLIST_HEAD (ObjectListHead, ObjectList) obj_list_head_t;
  // obj_list_head_t head = send_buffer_raw->list;
  obj_list_head_t *send_buffer_head = LIST_OBJECT_HEAD (send_buffer);
  obj_list_t send_buffer_node = SLIST_FIRST (send_buffer_head);

  u8_t *send_buffer_array = (u8_t *)os_malloc ((imm_int_t) (len_ptr->value));
  if (!send_buffer_array)
    {
      ret = &GLOBAL_REF (false_const);
      return ret;
    }

  u32_t idx = 0;
  SLIST_FOREACH (send_buffer_node, send_buffer_head, next)
  {
    // VALIDATE (send_buffer_node->obj, imm_int);
    imm_int_t v = (uint8_t)send_buffer_node->obj;
    if (!(v < 256 && v >= 0))
      {
        ret = &GLOBAL_REF (false_const);
        return ret;
      }
    send_buffer_array[idx] = (u8_t)v;
    idx++;
  }

  int status = 0;
  if (status != 0)
    ret = &GLOBAL_REF (false_const);
  else
    ret = &GLOBAL_REF (none_const);
  return ret;
}

static object_t _os_i2c_read_list (vm_t vm, object_t ret, object_t dev,
                                   object_t i2c_addr, object_t length)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  VALIDATE (length, imm_int);
  os_printk ("i2c_reg_read_list (%s, 0x%02X, %d)\n", (const char *)dev->value,
             (imm_int_t)i2c_addr->value, (imm_int_t)length->value);
  ret = &GLOBAL_REF (false_const);
  return ret;
}

static object_t _os_i2c_write_list (vm_t vm, object_t ret, object_t dev,
                                    object_t i2c_addr, object_t lst)
{
  VALIDATE (ret, imm_int);
  VALIDATE (dev, symbol);
  VALIDATE (i2c_addr, imm_int);
  // VALIDATE (reg_addr, imm_int);
  VALIDATE (lst, list);
  os_printk ("i2c_reg_write_list (%s, 0x%02X, ", (const char *)dev->value,
             (imm_int_t)i2c_addr->value);
  object_printer (lst);
  os_printk (")\n");
  ret = &GLOBAL_REF (false_const);
  return ret;
}

#endif /* LAMBDACHIP_LINUX */

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
  // #ifdef LAMBDACHIP_ZEPHYR
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
  def_prim (40, "null?", 1, (void *)prim_is_null);
  def_prim (41, "pair?", 1, (void *)prim_is_pair);
  def_prim (42, "spi_transceive", 4, (void *)_os_spi_transceive);
  def_prim (43, "i2c_read_list", 3, (void *)_os_i2c_read_list);
  def_prim (44, "i2c_write_list", 3, (void *)_os_i2c_write_list);

  // // gpio_pin_set(dev_led0, LED0_PIN, (((cnt) % 5) == 0) ? 1 : 0);

  // #endif /* LAMBDACHIP_ZEPHYR */
}

char *prim_name (u16_t pn)
{
#if defined LAMBDACHIP_DEBUG
  if (pn >= PRIM_MAX)
    {
      VM_DEBUG ("Invalid prim number: %d\n", pn);
      panic ("prim_name halt\n");
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
  for (int i = 0; i <= object_print; i++)
    {
      os_free (GLOBAL_REF (prim_table)[i]);
      GLOBAL_REF (prim_table)[i] = NULL;
    }
}
