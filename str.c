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
  ListHead *head = LIST_OBJECT_HEAD (lst);
  list_node_t node = NULL;
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

object_t _make_string (vm_t vm, object_t ret, object_t length, object_t char0)
{
  VALIDATE (length, imm_int);
  VALIDATE (char0, character);
  // FIXME: what if length larger than 2^31-1
  imm_int_t len = (imm_int_t)length->value;
  imm_int_t cc = (imm_int_t)char0->value;
  char c = '\0';
  if (CHAR_VALUE_VALID (cc))
    {
      c = (char)cc;
    }
  else
    {
      PANIC ("LambdaChip VM doesn't support #\nul in string. (R7RS allows such "
             "a design)\n");
    }

  // FIXME: Memory leaks here, there's no good way to free memory at this stage.
  char *p = (char *)GC_MALLOC (len + 1);
  if (p)
    {
      memset (p, c, len);
      p[len] = '\0';
    }
  ret->value = (void *)p;
  ret->attr.type = mut_string;
  return ret;
}

object_t _string (vm_t vm, object_t ret, object_t length, object_t char0)
{
  return ret;
}

object_t _string_length (vm_t vm, object_t ret, object_t obj)
{
  VALIDATE_STRING (obj);
  imm_int_t len = strnlen ((char *)obj->value, MAX_STR_LEN);

  ret->value = imm_int;
  ret->value = (void *)len;
  return ret;
}

// In R6RS string_ref shall run in constant time while R7RS doesn't
object_t _string_ref (vm_t vm, object_t ret, object_t obj, object_t index)
{
  VALIDATE_STRING (obj);
  VALIDATE (index, imm_int);

  imm_int_t idx = (imm_int_t)index->value;
  imm_int_t len = strnlen ((char *)obj->value, MAX_STR_LEN);

  if (idx < 0 || idx > len) // len is always less than MAX_STR_LEN
    {
      PANIC ("String index error, string_length = %d, index = %d\n", len, idx);
    }
  ret->value = (void *)((char *)obj->value)[idx];
  ret->attr.type = character;
  return ret;
}

object_t _string_set (vm_t vm, object_t ret, object_t obj, object_t index,
                      object_t char0)
{
  // only mut_string is allowed, string type is not allowed.
  VALIDATE (obj, mut_string);
  VALIDATE (index, imm_int);
  VALIDATE (char0, character);

  imm_int_t idx = (imm_int_t)index->value;
  imm_int_t cc = (imm_int_t)char0->value;
  if (!CHAR_VALUE_VALID (cc))
    {
      PANIC ("Char value (%d) not in range (%d, %d)\n", cc, MIN_CHAR, MAX_CHAR);
    }

  ((char *)obj->value)[idx] = cc;
  ret->attr.type = none;
  ret->value = (void *)0;

  return ret;
}

object_t _string_eq (vm_t vm, object_t ret, object_t str0, object_t str1)
{
  VALIDATE_STRING (str0);
  VALIDATE_STRING (str1);

  // ret->attr.type = boolean;
  if (str0->value == str1->value)
    {
      // ret->value = (void*)true;
      *ret = GLOBAL_REF (true_const);
    }

  if (0 == strncmp ((char *)str0->value, (char *)str1->value, MAX_STR_LEN))
    {
      // ret->value = (void*)true;
      *ret = GLOBAL_REF (true_const);
    }
  else
    {
      *ret = GLOBAL_REF (false_const);
      // ret->value = (void*)false;
    }
  return ret;
}

object_t _substring (vm_t vm, object_t ret, object_t str0, object_t start,
                     object_t end)
{
  VALIDATE_STRING (str0);
  VALIDATE (start, imm_int);
  VALIDATE (end, imm_int);

  imm_int_t len = strnlen ((char *)str0->value, MAX_STR_LEN);

  imm_int_t s = (imm_int_t)start->value;
  imm_int_t e = (imm_int_t)end->value;

  if (0 > s || s > len)
    {
      PANIC ("Value out of range %d to %d: %d", 0, len, s);
    }

  if (e < s || e > len)
    {
      PANIC ("Value out of range %d to %d: %d", s, len, e);
    }

  imm_int_t new_len = e - s;

  // FIXME: Memory leaks here, there's no good way to free memory at this stage.
  char *p = (char *)GC_MALLOC (new_len + 1);
  p[new_len] = '\0';
  strncpy (p, (char *)str0->value, new_len);

  ret->attr.type = mut_string;
  ret->value = (void *)p;
  return ret;
}

object_t _string_append (vm_t vm, object_t ret, object_t str0, object_t str1)
{
  VALIDATE_STRING (str0);
  VALIDATE_STRING (str1);
  // VALIDATE (start, imm_int);
  // VALIDATE (end, imm_int);

  imm_int_t len0 = strnlen ((char *)str0->value, MAX_STR_LEN);
  imm_int_t len1 = strnlen ((char *)str1->value, MAX_STR_LEN);

  // FIXME: Memory leaks here, there's no good way to free memory at this stage.
  char *p = (char *)GC_MALLOC (len0 + len1 + 1);
  if (p)
    {
      strncpy (p, (char *)str0->value, len0);
      strncpy (p + len0, (char *)str1->value, len1);
    }
  else
    {
      PANIC ("Memory allocation failed.");
    }

  ret->attr.type = mut_string;
  ret->value = (void *)p;
  return ret;
}

object_t _string_copy (vm_t vm, object_t ret, object_t str0, object_t start,
                       object_t end)
{
  return _substring (vm, ret, str0, start, end);
}

object_t _string_copy_side_effect (vm_t vm, object_t ret, object_t str0,
                                   object_t at, object_t str1, object_t start,
                                   object_t end)
{
  VALIDATE_STRING (str0);
  VALIDATE (at, imm_int);
  VALIDATE_STRING (str1);
  VALIDATE (start, imm_int);
  VALIDATE (end, imm_int);

  imm_int_t len0 = strnlen ((char *)str0->value, MAX_STR_LEN);
  imm_int_t len1 = strnlen ((char *)str0->value, MAX_STR_LEN);

  imm_int_t a = (imm_int_t)at->value;
  imm_int_t s = (imm_int_t)start->value;
  imm_int_t e = (imm_int_t)end->value;

  if (0 > a || a > len0)
    {
      PANIC ("Value out of range %d to %d: %d", 0, len0, a);
    }

  if (0 > s || s > len1)
    {
      PANIC ("Value out of range %d to %d: %d", 0, len1, s);
    }

  if (e < s || e > len1)
    {
      PANIC ("Value out of range %d to %d: %d", s, len1, e);
    }

  if (len0 - a < e - s)
    {
      PANIC ("In procedure string-copy!: Argument 3 out of range: %s",
             (char *)str1->value);
    }

  imm_int_t new_len = e - s;

  // FIXME: Memory leaks here, there's no good way to free memory at this stage.
  char *p = (char *)GC_MALLOC (new_len + 1);
  p[new_len] = '\0';
  strncpy (a + (char *)str0->value, s + (char *)str1->value, e - s);

  ret->attr.type = none;
  ret->value = (void *)0;
  return ret;
}

object_t _string_fill (vm_t vm, object_t ret, object_t str0, object_t fill,
                       object_t start, object_t end)
{
  VALIDATE_STRING (str0);
  VALIDATE (fill, character);
  VALIDATE (start, imm_int);
  VALIDATE (end, imm_int);

  imm_int_t len = strnlen ((char *)str0->value, MAX_STR_LEN);

  char c = (char)fill->value;
  imm_int_t s = (imm_int_t)start->value;
  imm_int_t e = (imm_int_t)end->value;

  if (0 > s || s > len)
    {
      PANIC ("Value out of range %d to %d: %d", 0, len, s);
    }

  if (e < s || e > len)
    {
      PANIC ("Value out of range %d to %d: %d", s, len, e);
    }

  char *p = (char *)str0->value;
  for (imm_int_t i = s; i < e; i++)
    {
      p[i] = c;
    }

  ret->attr.type = none;
  ret->value = (void *)0;
  return ret;
}
