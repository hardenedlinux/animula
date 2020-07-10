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

#include "primitives.h"
#include "lib.h"

prim_t __prim_table[PRIM_MAX] = {0};

void primitives_init(void)
{
  def_prim(1, "add", 2, int_add);
}

#if defined LAMBDACHIP_DEBUG
char* prim_name(u16_t pn)
{
  if (pn >= PRIM_MAX)
    {
      VM_DEBUG("Invalid prim number: %d\n", pn);
      panic("prim_name halt\n");
    }

  return __prim_table[pn]->name;
}
#endif

prim_t get_prim(u16_t pn)
{
  return __prim_table[pn];
}
