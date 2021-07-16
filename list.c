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

#include "list.h"

object_t _car (vm_t vm, object_t ret, object_t obj)
{
  switch (obj->attr.type)
    {
    case list:
      {
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);
        obj_list_t first = SLIST_FIRST (head);
        *ret = *(first->obj);
        break;
      }
    case pair:
      {
        *ret = *(((pair_t)obj->value)->car);
        break;
      }
    default:
      {
        os_printk ("car: Invalid object type %d\n", obj->attr.type);
        PANIC ("The program is down!\n");
      }
    }

  return ret;
}

object_t _cdr (vm_t vm, object_t ret, object_t obj)
{
  switch (obj->attr.type)
    {
    case list:
      {
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);
        obj_list_t first = SLIST_FIRST (head);
        obj_list_t next_node = SLIST_NEXT (first, next);

        if (next_node)
          {
            ret->attr.gc = 0;
            ret->attr.type = list;
            list_t l = NEW_INNER_OBJ (list);
            SLIST_INIT (&l->list);
            ret->value = (void *)l;
            obj_list_head_t *new_head = LIST_OBJECT_HEAD (ret);
            new_head->slh_first = next_node;
          }
        else
          {
            *ret = GLOBAL_REF (null_const);
          }
        break;
      }
    case pair:
      {
        *ret = *(((pair_t)obj->value)->cdr);
        break;
      }
    default:
      {
        PANIC ("cdr: Invalid object type %d\n", obj->attr.type);
      }
    }

  return ret;
}

object_t _cons (vm_t vm, object_t ret, object_t a, object_t b)
{
  switch (b->attr.type)
    {
    case null_obj:
      {
        ret->attr.type = list;
        list_t lst = NEW_INNER_OBJ (list);
        obj_list_t ol = NEW_LIST_NODE ();
        SLIST_INSERT_HEAD (&lst->list, ol, next);
        ret->value = (void *)lst;
        break;
      }
    default:
      {
        pair_t p = NEW_INNER_OBJ (pair);
        object_t new_a = OBJ_IS_ON_STACK (a) ? NEW_OBJ (0) : a;
        object_t new_b = OBJ_IS_ON_STACK (b) ? NEW_OBJ (0) : b;
        if (new_a != a)
          *new_a = *a;
        if (new_b != b)
          *new_b = *b;
        p->car = new_a;
        p->cdr = new_b;
        ret->attr.type = pair;
        ret->value = (void *)p;
      }
    }

  return ret;
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

object_t _list_ref (vm_t vm, object_t ret, object_t lst, object_t idx)
{
  VALIDATE (lst, list);
  VALIDATE (idx, imm_int);

  obj_list_head_t *head = LIST_OBJECT_HEAD (lst);
  obj_list_t node = NULL;
  imm_int_t cnt = (imm_int_t)idx->value;
  imm_int_t lst_idx = cnt;

  if (SLIST_EMPTY (head))
    {
      PANIC ("list-ref encounter an empty List!\n");
    }

  if (cnt < 0)
    {
      PANIC ("Invalid index %d!\n", lst_idx);
    }

  SLIST_FOREACH (node, head, next)
  {
    if (!cnt)
      break;
    cnt--;
  }

  if (cnt < 0)
    {
      PANIC ("BUG: wrong counter, %d!\n", lst_idx);
    }

  if (!node)
    {
      PANIC ("Invalid index %d!\n", lst_idx);
      // FIXME: implement throw
      // throw ();
    }

  *ret = *(node->obj);
  return ret;
}

object_t _list_set (vm_t vm, object_t ret, object_t lst, object_t idx,
                    object_t val)
{
  VALIDATE (lst, mut_list);
  VALIDATE (idx, imm_int);

  obj_list_t node = NULL;
  obj_list_head_t *head = LIST_OBJECT_HEAD (lst);
  imm_int_t cnt = (imm_int_t)idx->value;

  if (SLIST_EMPTY (head))
    {
      PANIC ("list-set! encounter an empty List!\n");
    }

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
      PANIC ("list-set!: Invalid index %d!\n", cnt);
      // FIXME: implement throw
      // throw ();
    }

  *ret = GLOBAL_REF (none_const);
  return ret;
}

object_t _list_append (vm_t vm, object_t ret, object_t l1, object_t l2)
{
  VALIDATE (l1, list);
  VALIDATE (l2, list);

  ret->attr.type = list;
  list_t l = NEW_INNER_OBJ (list);
  SLIST_INIT (&l->list);
  ret->value = (void *)l;
  obj_list_head_t *new_head = LIST_OBJECT_HEAD (ret);

  if (list == l2->attr.type)
    {
      obj_list_head_t *h1 = LIST_OBJECT_HEAD (l1);
      obj_list_head_t *h2 = LIST_OBJECT_HEAD (l2);
      obj_list_t node = NULL;
      obj_list_t prev = NULL;

      /* NOTE:
       * According to r7rs, the element objects should be shared with the
       * original lists. However, we still need to allocate new node for each of
       * them.
       */
      SLIST_FOREACH (node, h1, next)
      {
        obj_list_t new_node = NEW_LIST_NODE ();
        new_node->obj = node->obj;

        if (!prev)
          {
            // when the new list is still empty
            SLIST_INSERT_HEAD (new_head, new_node, next);
          }
        else
          {
            SLIST_INSERT_AFTER (prev, new_node, next);
          }
        prev = new_node;
      }

      SLIST_FOREACH (node, h2, next)
      {
        obj_list_t new_node = NEW_LIST_NODE ();
        new_node->obj = node->obj;
        SLIST_INSERT_AFTER (prev, new_node, next);
        prev = new_node;
      }
    }
  else
    {
      return _cons (vm, ret, l1, l2);
    }

  return ret;
}

object_t _list_length (vm_t vm, object_t ret, object_t l1)
{
  VALIDATE (l1, list);
  ret->attr.type = imm_int;
  list_t lst = (list_t)l1->value;
  obj_list_head_t *head = LIST_OBJECT_HEAD (l1);
  obj_list_t n1 = SLIST_FIRST (head);
  imm_int_t len = 0;
  SLIST_FOREACH (n1, head, next)
  {
    len++;
  }
  ret->value = (void *)len;
  return ret;
}
