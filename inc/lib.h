#ifndef __ANIMULA_LIB_H__
#define __ANIMULA_LIB_H__
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

#include "debug.h"
#include "object.h"
#include "os.h"
#include "primitives.h"
#include "types.h"

static inline imm_int_t gcd (imm_int_t u, imm_int_t v)
{

  imm_int_t uu = abs (u);
  imm_int_t vv = abs (v);

  while ((uu %= vv) && (vv %= uu))
    ;
  return (uu + vv);
}

static inline s64_t gcd64 (s64_t u, s64_t v)
{
  s64_t uu = (u >= 0) ? u : (-1 * u);
  s64_t vv = (v >= 0) ? v : (-1 * v);
  while ((uu %= vv) && (vv %= uu))
    ;
  return (uu + vv);
}

static inline s64_t abs64 (s64_t x)
{
  // due to the asymmetric nature of signed int, (s32_t)0xFFFFFFFF
  // cannot convert to a valid s32_t integer. abs (0xFFFFFFFF) = 0x100000000
  if (x == MIN_INT32)
    {
      os_printk (
        "%s:%d, %s: Out of range, cannot calculate opposite number of %lld,\n",
        __FILE__, __LINE__, __FUNCTION__, x);
      panic ("");
      return -x;
    }
  else if (x > 0)
    {
      return x;
    }
  else
    {
      return -x;
    }
}

#endif // End of __ANIMULA_LIB_H__
