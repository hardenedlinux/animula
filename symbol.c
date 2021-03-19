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

#include "symbol.h"

GLOBAL_DEF (symtab_t, symtab) = NULL;

static RB_HEAD (SymbolInternTable, SymbolNode)
  SymbolInternTableHead = RB_INITIALIZER (&SymbolInternTableHead);

RB_GENERATE_STATIC (SymbolInternTable, SymbolNode, entry,
                    intern_symbol_compare);

static void intern (const char *str_buf)
{
  SymbolNode *node = (SymbolNode *)os_malloc (sizeof (SymbolNode));
  node->str_buf = str_buf;
  RB_INSERT (SymbolInternTable, &SymbolInternTableHead, node);
}

static SymbolNode *is_interned (const char *str_buf)
{
  SymbolNode node = {.str_buf = str_buf};
  return RB_FIND (SymbolInternTable, &SymbolInternTableHead, &node);
}

/* NOTE:
 * 1. Symbols are not managed by GC and never be freed.
 * 2. We pass the return obj pointer to avoid copy.
 */
void make_symbol (const char *str_buf, object_t obj)
{
  SymbolNode *ret = is_interned (str_buf);

  if (!ret)
    intern (str_buf);

  obj->value = (void *)str_buf;
}

/* NOTE: We always create new object for pure functionality.
 */

Object symbol_to_string (object_t sym)
{
  VALIDATE (sym, symbol);

  Object obj = {.attr = {.type = string, .gc = 0}, .value = (void *)sym->value};

  return obj;
}

Object string_to_symbol (object_t str)
{
  VALIDATE (str, string);

  Object obj = {.attr = {.type = symbol, .gc = 0}, .value = (void *)str->value};

  return obj;
}

void create_symbol_table (symtab_t st)
{
  const char *str_buf = (const char *)st->entry;

  for (u16_t i = 0, start = 0; i < st->cnt; i++)
    {
      const char *str = str_buf + start;
      // os_printk ("intern: %s\n", str);
      intern (str);
      start += os_strnlen (str, MAX_STR_LEN) + 1; // skip '\0'
    }
}

bool symbol_eq (object_t a, object_t b)
{
  return a->value == b->value;
}
