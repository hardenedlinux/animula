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

#include "lib.h"

imm_int_t _int_add (imm_int_t x, imm_int_t y)
{
  return x + y;
}

imm_int_t _int_sub (imm_int_t x, imm_int_t y)
{
  printf ("%d - %d = %d\n", x, y, x - y);
  return x - y;
}

imm_int_t _int_mul (imm_int_t x, imm_int_t y)
{
  return x + y;
}

imm_int_t _int_div (imm_int_t x, imm_int_t y)
{
  return x / y;
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
