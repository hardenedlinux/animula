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

GLOBAL_DEF (prim_t, prim_table[PRIM_MAX]) = {0};

void primitives_init (void)
{
  def_prim (0, "ret", 0, NULL);
  def_prim (2, "add", 2, (void *)_int_add);
  def_prim (3, "sub", 2, (void *)_int_sub);
  def_prim (4, "mul", 2, (void *)_int_mul);
  def_prim (5, "div", 2, (void *)_int_div);
  def_prim (6, "object_print", 1, (void *)_object_print);
}

#if defined LAMBDACHIP_DEBUG
char *prim_name (u16_t pn)
{
  if (pn >= PRIM_MAX)
    {
      VM_DEBUG ("Invalid prim number: %d\n", pn);
      panic ("prim_name halt\n");
    }

  return GLOBAL_REF (prim_table)[pn]->name;
}
#endif

prim_t get_prim (u16_t pn)
{
  return GLOBAL_REF (prim_table)[pn];
}

void primitives_clean (void)
{
  /* Skip 0 (halt), and 1 (ret)
   */
  for (int i = int_add; i <= object_print; i++)
    {
      os_free (GLOBAL_REF (prim_table)[i]);
      GLOBAL_REF (prim_table)[i] = NULL;
    }
}
