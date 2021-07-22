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

#include "str.h"

bool str_eq (object_t s1, object_t s2)
{
  return (0 == os_strncmp (s1->value, s2->value, MAX_STR_LEN));
}

object_t _read_char (vm_t vm, object_t ret)
{
  char ch = os_getchar ();
  ret->attr.type = character;
  ret->value = (void *)ch;
  return ret;
}

object_t _read_str (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE (obj, imm_int);

  char ch;
  imm_int_t cnt = (imm_int_t)obj->value;
  char *buf = (char *)GC_MALLOC (cnt + 1);

  for (int i = 0; i < cnt; i++)
    {
      buf[i] = getchar ();
    }

  ret->attr.type = mut_string;
  ret->value = (void *)buf;

  return ret;
}

object_t _read_line (vm_t vm, object_t ret)
{

  char buf[MAX_STR_LEN] = {0};
  char ch;
  int cnt = 0;

  while ((buf[cnt++] = os_getchar ()) != '\n')
    ;

  buf[cnt] = '\0';
  char *str = (char *)GC_MALLOC (cnt);
  os_memcpy (str, buf, cnt);
  ret->attr.type = mut_string;
  ret->value = (void *)str;

  return ret;
}

object_t _list_to_string (vm_t vm, object_t ret, object_t lst)
{
  char buf[MAX_STR_LEN] = {0};
  obj_list_head_t *head = LIST_OBJECT_HEAD (lst);
  obj_list_t node = NULL;
  ret->attr.type = mut_string;
  int cnt = 0;

  SLIST_FOREACH (node, head, next)
  {
    buf[cnt++] = (char)node->obj->value;
  }

  buf[cnt] = '\0';
  char *str = (char *)GC_MALLOC (cnt);
  os_memcpy (str, buf, cnt);
  ret->attr.type = mut_string;
  ret->value = (void *)str;

  return ret;
}

Object prim_string_p (object_t obj)
{
  int type = obj->attr.type;
  return (((string == type) || (mut_string == type))
            ? GLOBAL_REF (true_const)
            : GLOBAL_REF (false_const));
}

Object prim_char_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, character);
}

Object prim_keyword_p (object_t obj)
{
  return CHECK_OBJECT_TYPE (obj, keyword);
}
