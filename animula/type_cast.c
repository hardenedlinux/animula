/*  Copyright (C) 2020-2021
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

#include "type_cast.h"
#include "types.h"

void cast_imm_int_to_rational (object_t ret)
{
  if (ret->attr.type == imm_int)
    {
      imm_int_t val = (imm_int_t)ret->value;
      if (val >= 0)
        {
          ret->attr.type = rational_pos;
          ret->value = (void *)(((u32_t)val << 16) | 1);
        }
      else
        {
          ret->attr.type = rational_neg;
          ret->value = (void *)(((u32_t)(-val) << 16) | 1);
        }
    }
}

void cast_rational_to_imm_int_if_denominator_is_1 (object_t v)
{
  if (v->attr.type == rational_pos || v->attr.type == rational_neg)
    {
      u32_t val = (u32_t)v->value;
      u16_t denominator = val & 0xFFFF;
      u16_t numerator = (val >> 16) & 0xFFFF;
      if (denominator == 1)
        {
          int sign = (v->attr.type == rational_pos) ? 1 : -1;
          v->attr.type = imm_int;
          v->value = (void *)(sign * (imm_int_t)numerator);
        }
    }
}

uintptr_t cast_rational_to_float (immu_object_t v)
{
  // This function doesn't modify the object, so it's fine to have const parameter
  // Implementation can be added later
  (void)v;
  return 0;
}
