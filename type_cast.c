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
#include "type_cast.h"
#include "object.h"
#include "os.h"

void cast_imm_int_to_rational (object_t v)
{
  oattr t;
  t.type = v->attr.type;
  if ((t.type == rational_pos) || (t.type == rational_neg))
    {
      return;
    }
  VALIDATE (v, imm_int);
  imm_int_t n = (imm_int_t)v->value;
  VALIDATE_NUMERATOR (n);

  if (n > 65535 || n < -65535)
    {
      PANIC ("Out of range cannot convert %d to rational\n",
             (imm_int_t)v->value);
    }
  if (n > 0)
    {
      v->attr.type = rational_pos;
      v->value = (void *)((((imm_int_t)v->value) << 16) | 1);
    }
  else // (n < 0)
    {
      v->attr.type = rational_neg;
      v->value = (void *)(((-1 * (imm_int_t)v->value) << 16) | 1);
    }
  return;
}

void cast_rational_to_imm_int_if_denominator_is_1 (object_t v)
{
  imm_int_t d = 0;
  imm_int_t n = 0;
  n = ((imm_int_t)v->value >> 16) & 0xFFFF;
  d = (imm_int_t)v->value & 0xFFFF;
  if (d != 1)
    {
      return;
    }
  imm_int_t sign;
  if (v->attr.type == rational_pos)
    {
      sign = 1;
    }
  else if (v->attr.type == rational_neg)
    {
      sign = -1;
    }
  else
    {
      PANIC ("TYPE_ERROR: type is %d\n", v->attr.type);
    }
  v->attr.type = imm_int;
  v->value = (void *)(sign * n);
  return;
}

// TODO: rename to __cast_rational_to_real
// side effect
void cast_rational_to_float (object_t v)
{
  if ((v->attr.type != rational_neg) && (v->attr.type != rational_pos))
    {
      PANIC ("Invalid type, expect %d or %d, but it's %d\n", rational_pos,
             rational_neg, v->attr.type);
    }
  int sign = (v->attr.type == rational_pos) ? 1 : -1;
  imm_int_t c = (((imm_int_t)v->value) >> 16) & 0xFFFF;
  imm_int_t d = ((imm_int_t)v->value) & 0xFFFF; // v = c/d
  float a = sign * c;
  real_t b = {0};
  b.f = a / d;
  v->attr.type = real;
  v->value = (void *)b.v;
}

// side effect
void cast_int_or_fractal_to_float (object_t v)
{
  oattr t;
  t.type = v->attr.type;
  // if (t.type == complex_inexact)
  // {}
  // else if (t.type == complex_exact)
  // {}
  if (t.type == real)
    {
      return;
    }
  else if ((t.type == rational_pos) || (t.type == rational_neg))
    {
      cast_rational_to_float (v);
    }
  else if (t.type == imm_int)
    {
      v->attr.type = real;
      imm_int_t b = (imm_int_t) (v->value);
      real_t a = {0};
      a.f = (float)b;
      v->value = (void *)a.v;
      return;
    }
  else
    {
      PANIC ("Invalid type, expect imm_int, rationa  or real, but it's %d\n",
             v->attr.type);
    }
}

void bit_order_checking (void)
{
  STATIC_ASSERT ((((imm_int_t)BIT_ORDER_MASK >> 16) & 0xFFFF)
                 == (imm_int_t)BIT_ORDER_MASK2);
  STATIC_ASSERT (((imm_int_t)BIT_ORDER_MASK | (imm_int_t)1)
                 == (imm_int_t)BIT_ORDER_MASK3);
}
