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

#include "print.h"

static inline void list_printer (const object_t obj)
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

static inline void pair_printer (const object_t obj)
{
  os_printk ("(");
  object_printer (((pair_t)obj->value)->car);
  os_printk (" . ");
  object_printer (((pair_t)obj->value)->cdr);
  os_printk (")");
}

static inline void vector_printer (const object_t obj)
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

static inline void rational_printer (const object_t obj)
{
  hov_t value = (hov_t)obj->value;
  numerator_t n = (numerator_t) (value >> 16);
  denominator_t d = (denominator_t) (value & 0xffff);
  if (obj->attr.type == rational_neg)
    {
      os_printk ("-%d/%d", n > 0 ? n : -n, d);
    }
  else
    {
      os_printk ("%d/%d", n > 0 ? n : -n, d);
    }
}

static inline void complex_exact_printer (const object_t obj)
{
  hov_t value = (hov_t)obj->value;
  real_part_t r = (real_part_t) (value >> 0xf);
  imag_part_t i = (imag_part_t) (value & 0xf);

  if (i >= 0)
    {
      os_printk ("%d+%di", r, i);
    }
  else
    {
      os_printk ("%d%di", r, i);
    }
}

static inline void complex_inexact_printer (const object_t obj)
{
  real_t r = {.v = read_uintptr_from_ptr ((char *)obj->value)};
  real_t i = {.v = read_uintptr_from_ptr ((char *)obj->value + 4)};

  if (i.f >= 0)
    {
      os_printk ("%.1f+%.1fi", r.f, i.f);
    }
  else
    {
      os_printk ("%.1f%.1fi", r.f, i.f);
    }
}

void object_printer (const object_t obj)
{
  switch (obj->attr.type)
    {
    case imm_int:
      {
        os_printk ("%d", (imm_int_t)obj->value);
        break;
      }
    case string:
    case mut_string:
    case symbol:
      {
        os_printk ("%s", (char *)obj->value);
        break;
      }
    case keyword:
      {
        os_printk ("#:%s", (char *)obj->value);
        break;
      }
    case character:
      {
        os_printk ("#\\%c", (char)obj->value);
        break;
      }
    case primitive:
      {
        u16_t pn = (u16_t)obj->value;
        os_printk ("<primitive: %s %d>", prim_name (pn), pn);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        os_printk ("<closure: %p>", obj->value);
        break;
      }
    case procedure:
      {
        os_printk ("<procedure: %p>", obj->value);
        break;
      }
    case pair:
      {
        pair_printer (obj);
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
    case real:
      {
        real_t r = {.v = (uintptr_t)obj->value};
        os_printk ("float: %f", r.f);
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        rational_printer (obj);
        break;
      }
    case complex_exact:
      {
        complex_exact_printer (obj);
        break;
      }
    case complex_inexact:
      {
        complex_inexact_printer (obj);
        break;
      }
    case null_obj:
      {
        os_printk ("()");
        break;
      }
    case none:
      {
        os_printk ("<unspecified>");
        break;
      }
    default:
      {
        os_printk ("object_print: Invalid object type %d\n", obj->attr.type);
        panic ("VM stop!");
      }
    }
}
