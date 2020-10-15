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

#include "print.h"

static inline void list_printer (object_t obj)
{
  obj_list_head_t head = ((list_t)obj->value)->list;
  obj_list_t node = NULL;

  os_printk ("(");
  SLIST_FOREACH (node, &head, next)
  {
    object_printer (node->obj);
    if (SLIST_NEXT (node, next))
      os_printk (" ");
  }
  os_printk (")");
}

static inline void vector_printer (object_t obj)
{
  u16_t size = ((vector_t)obj->value)->size;
  os_printk ("#(");
  for (u16_t i = 0; i < size; i++)
    {
      object_printer (obj);
      if (i < size - 1)
        os_printk (" ");
    }
  os_printk (")");
}

void object_printer (object_t obj)
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
    case symbol:
      {
        const char *str_buf = GET_SYMBOL ((u32_t)obj->value);
        os_printk ("#{%s}#", str_buf);
        break;
      }
    case primitive:
      {
        u16_t pn = (u16_t)obj->value;
        os_printk ("<primitive: %s %d>", prim_name (pn), pn);
        break;
      }
    case list:
      {
        list_printer (obj);
        break;
      }
    case vector:
      {
        vector_printer (obj);
        break;
      }
    case boolean:
      {
        os_printk ("#%s", obj->value ? "true" : "false");
        break;
      }
    default:
      {
        os_printk ("object_print: Invalid object type %d\n", obj->attr.type);
        panic ("PANIC!\n");
      }
    }
}
