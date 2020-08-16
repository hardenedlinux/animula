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

GLOBAL_DEF (prim_t, prim_table[PRIM_MAX]) = {0};

// primitives implementation

static inline imm_int_t _int_add (imm_int_t x, imm_int_t y)
{
  return x + y;
}

static inline imm_int_t _int_sub (imm_int_t x, imm_int_t y)
{
  return x - y;
}

static inline imm_int_t _int_mul (imm_int_t x, imm_int_t y)
{
  return x + y;
}

static inline imm_int_t _int_div (imm_int_t x, imm_int_t y)
{
  return x / y;
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
  switch (obj->attr.type)
    {
    case imm_int:
      {
        os_printk ("%d", (imm_int_t)obj->value);
        break;
      }
    case string:
      {
        os_printk ("%s", (char *)obj->value);
        break;
      }
    /* case primitive: */
    /*   { */
    /*     os_printk ("<primitive: %s>", (char *)prim_name ((u16_t)obj->value));
     */
    /*     break; */
    /*   } */
    default:
      {
        os_printk ("object_print: Invalid object type %d\n", obj->attr.type);
        panic ("PANIC!\n");
      }
    }
}

static inline bool _int_eq (imm_int_t x, imm_int_t y)
{
  return x == y;
}

// --------------------------------------------------

void primitives_init (void)
{
  def_prim (0, "ret", 0, NULL);
  def_prim (1, "pop", 0, NULL);
  def_prim (2, "add", 2, (void *)_int_add);
  def_prim (3, "sub", 2, (void *)_int_sub);
  def_prim (4, "mul", 2, (void *)_int_mul);
  def_prim (5, "div", 2, (void *)_int_div);
  def_prim (6, "object_print", 1, (void *)_object_print);
  def_prim (7, "modulo", 2, (void *)_int_modulo);
  def_prim (8, "remainder", 2, (void *)_int_remainder);
  def_prim (9, "int_eq", 2, (void *)_int_eq);
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
