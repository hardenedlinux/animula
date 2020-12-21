#ifndef __LAMBDACHIP_LIB_H__
#define __LAMBDACHIP_LIB_H__
/*  Copyright (C) 2020
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

#include "debug.h"
#include "object.h"
#include "os.h"
#include "primitives.h"
#include "types.h"

static inline imm_int_t gcd (imm_int_t u, imm_int_t v)
{

  imm_int_t uu = abs (u);
  imm_int_t vv = abs (v);

  while ((u %= v) && (v %= u))
    ;
  return (u + v);
}
#endif // End of __LAMBDACHIP_LIB_H__
