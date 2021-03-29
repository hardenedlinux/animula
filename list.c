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

#include "list.h"

object_t _car (object_t obj)
{
  switch (obj->attr.type)
    {
    case list:
      {
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);
        obj_list_t first = SLIST_FIRST (head);
        return first->obj;
      }
    case pair:
      {
        return ((pair_t)obj->value)->car;
      }
    default:
      {
        os_printk ("car: Invalid object type %d\n", obj->attr.type);
        panic ("The program is down!\n");
      }
    }

  return NULL;
}

object_t _cdr (object_t obj)
{
  switch (obj->attr.type)
    {
    case list:
      {
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);
        obj_list_t first = SLIST_FIRST (head);
        obj_list_t next = SLIST_NEXT (first, next);

        if (next)
          {
            return next->obj;
          }
        else
          {
            return &GLOBAL_REF (null_const);
          }
      }
    case pair:
      {
        return ((pair_t)obj->value)->cdr;
      }
    default:
      {
        os_printk ("cdr: Invalid object type %d\n", obj->attr.type);
        panic ("The program is down!\n");
      }
    }

  return NULL;
}

object_t _cons (object_t a, object_t b)
{
  object_t obj = lambdachip_new_object (pair);

  switch (b->attr.type)
    {
    case null_obj:
      {
        obj->attr.type = list;
        list_t lst = lambdachip_new_list ();
        obj_list_t ol = new_obj_list ();
        SLIST_INSERT_HEAD (&lst->list, ol, next);
        obj->value = (void *)lst;
        break;
      }
    default:
      {
        pair_t p = lambdachip_new_pair ();
        p->car = a;
        p->cdr = b;
      }
    }

  return obj;
}

bool _is_pair (object_t obj)
{
  switch (obj->attr.type)
    {
    case list:
    case pair:
      return true;
    default:
      return false;
    }
}

object_t _list_ref (object_t lst, object_t idx)
{
  VALIDATE (lst, list);
  VALIDATE (idx, imm_int);

  obj_list_head_t *head = LIST_OBJECT_HEAD (lst);
  obj_list_t node = NULL;
  imm_int_t cnt = (imm_int_t)idx->value;
  object_t ret = &GLOBAL_REF (null_const);

  SLIST_FOREACH (node, head, next)
  {
    if (!cnt)
      ret = node->obj;

    cnt--;
  }

  if (!ret)
    {
      os_printk ("list-ref: Invalid index %d!\n", cnt);
      panic ("");
      // FIXME: implement throw
      // throw ();
    }

  return ret;
}

object_t _list_set (object_t lst, object_t idx, object_t val)
{
  VALIDATE (lst, list);
  VALIDATE (idx, imm_int);

  obj_list_t node = NULL;
  obj_list_head_t *head = LIST_OBJECT_HEAD (lst);
  imm_int_t cnt = (imm_int_t)idx->value;
  object_t ret = val;

  SLIST_FOREACH (node, head, next)
  {
    if (!cnt)
      {
        node->obj = (void *)val;
        break;
      }

    cnt--;
  }

  if (node->obj != val)
    {
      os_printk ("list-set!: Invalid index %d!\n", cnt);
      panic ("");
      // FIXME: implement throw
      // throw ();
    }

  return ret;
}

object_t _list_append (object_t l1, object_t l2)
{
  VALIDATE (l1, list);
  VALIDATE (l2, list);

  if (list == l2->attr.type)
    {
      obj_list_head_t *h1 = LIST_OBJECT_HEAD (l1);
      obj_list_head_t *h2 = LIST_OBJECT_HEAD (l2);
      obj_list_t node = NULL;
      obj_list_t tail = NULL;

      SLIST_FOREACH (node, h1, next)
      {
        tail = node;
      }

      SLIST_NEXT (tail, next) = SLIST_FIRST (h2);
    }
  else
    {
      return _cons (l1, l2);
    }

  return l1;
}
