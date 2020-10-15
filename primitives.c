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
  return x * y;
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
  object_printer (obj);
}

static bool _int_eq (object_t x, object_t y)
{
  // printf ("%d == %d is %d\n", x, y, x == y);
  return (imm_int_t)x->value == (imm_int_t)y->value;
}

static bool _int_lt (object_t x, object_t y)
{
  return (imm_int_t)x->value < (imm_int_t)y->value;
}

static bool _int_gt (object_t x, object_t y)
{
  return (imm_int_t)x->value > (imm_int_t)y->value;
}

static bool _int_le (object_t x, object_t y)
{
  return (imm_int_t)x->value <= (imm_int_t)x->value;
}

static bool _int_ge (object_t x, object_t y)
{
  return (imm_int_t)x->value >= (imm_int_t)x->value;
}
// --------------------------------------------------

static bool _not (object_t obj)
{
  return is_false (obj);
}

static bool _eq (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;

  if (t1 != t2)
    {
      return false;
    }
  else if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
    {
      return true;
    }
  else if (procedure == t1)
    {
      return (a->proc.entry == b->proc.entry);
    }

  return (a->value == b->value);
}

static bool _eqv (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
      /* case record: */
      /* case bytevector: */
      /* case float: */
    case imm_int:
      {
        ret = _int_eq (a, b);
        break;
      }
    case symbol:
      {
        ret = symbol_eq (a, b);
        break;
      }
    case boolean:
      {
        ret = (a == b);
        break;
      }
    case pair:
    case string:
    case vector:
    case list:
      {
        if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
          {
            ret = true;
          }
        else
          {
            /* NOTE:
             * For collections, eqv? only compare their head pointer.
             */
            ret = (a->value == b->value);
          }
        break;
      }
    case procedure:
      {
        /* NOTE:
         * R7RS demands the procedure that has side-effects for
         * different behaviour or return values to be the different
         * object. This is guaranteed by the compiler, not by the VM.
         */
        ret = (a->proc.entry == b->proc.entry);
        break;
      }
    default:
      {
        os_printk ("eqv?: The type %d hasn't implemented yet\n", t1);
        panic ("PANIC!\n");
      }
      // TODO-1
      /* case character: */
      /*   { */
      /*     ret = character_eq (a, b); */
      /*     break; */
      /*   } */

      // TODO-2
      // Inexact numbers
    }

  return ret;
}

static bool _equal (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
    case pair:
      {
        pair_t ap = (pair_t)a->value;
        pair_t bp = (pair_t)b->value;
        ret = (_equal (ap->car, bp->car) && _equal (ap->cdr, bp->cdr));
        break;
      }
    case string:
      {
        ret = str_eq (a, b);
        break;
      }
    case list:
      {
        if (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b))
          {
            ret = true;
          }
        else
          {
            obj_list_head_t *h1 = LIST_OBJECT_HEAD (a);
            obj_list_head_t *h2 = LIST_OBJECT_HEAD (b);
            obj_list_t n1 = NULL;
            obj_list_t n2 = SLIST_FIRST (h2);

            SLIST_FOREACH (n1, h1, next)
            {
              ret = _equal (n1->obj, n2->obj);
              if (!ret)
                break;
              n2 = SLIST_NEXT (n2, next);
            }
          }
        break;
      }
    case vector:
      {
        panic ("equal?: vector hasn't been implemented yet!\n");
        /* TODO: iterate each element and call equal? */
        break;
      }
      /* case bytevector: */
      /*   { */
      /*     ret = bytevector_eq (a, b); */
      /*     break; */
      /*   } */
    default:
      {
        ret = _eqv (a, b);
        break;
      }
    }

  return ret;
}

void primitives_init (void)
{
  /* NOTE: If fn is NULL, then it's inlined to call_prim
   */
  def_prim (0, "ret", 0, NULL);
  def_prim (1, "pop", 0, NULL);
  def_prim (2, "add", 2, (void *)_int_add);
  def_prim (3, "sub", 2, (void *)_int_sub);
  def_prim (4, "mul", 2, (void *)_int_mul);
  def_prim (5, "div", 2, (void *)_int_div);
  def_prim (6, "object_print", 1, (void *)_object_print);
  def_prim (7, "apply", 2, NULL);
  def_prim (8, "not", 1, (void *)_not);
  def_prim (9, "int_eq", 2, (void *)_int_eq);
  def_prim (10, "int_lt", 2, (void *)_int_lt);
  def_prim (11, "int_gt", 2, (void *)_int_gt);
  def_prim (12, "int_le", 2, (void *)_int_le);
  def_prim (13, "int_ge", 2, (void *)_int_ge);
  def_prim (14, "restore", 0, NULL);
  def_prim (15, "reserved_1", 0, NULL);

  // extended primitives
  def_prim (16, "modulo", 2, (void *)_int_modulo);
  def_prim (17, "remainder", 2, (void *)_int_remainder);
  def_prim (18, "foreach", 2, NULL);
  def_prim (19, "map", 2, NULL);
  def_prim (20, "list_ref", 2, (void *)list_ref);
  def_prim (21, "list_set", 3, (void *)list_set);
  def_prim (22, "list_append", 2, (void *)list_append);
  def_prim (23, "eq", 2, (void *)_eq);
  def_prim (24, "eqv", 2, (void *)_eqv);
  def_prim (25, "equal", 2, (void *)_equal);
}

char *prim_name (u16_t pn)
{
#if defined LAMBDACHIP_DEBUG
  if (pn >= PRIM_MAX)
    {
      VM_DEBUG ("Invalid prim number: %d\n", pn);
      panic ("prim_name halt\n");
    }

  return GLOBAL_REF (prim_table)[pn]->name;
#else
  return "";
#endif
}

prim_t get_prim (u16_t pn)
{
  return GLOBAL_REF (prim_table)[pn];
}

void primitives_clean (void)
{
  for (int i = 0; i <= object_print; i++)
    {
      os_free (GLOBAL_REF (prim_table)[i]);
      GLOBAL_REF (prim_table)[i] = NULL;
    }
}
