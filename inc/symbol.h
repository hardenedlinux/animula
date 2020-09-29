#ifndef __LAMBDACHIP_SYMBOL_H__
#define __LAMBDACHIP_SYMBOL_H__
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

#include "object.h"
#include "os.h"
#include "rbtree.h"
#include "types.h"

typedef struct SymTab
{
  u16_t cnt;
  u8_t *entry;
} __packed *symtab_t, symtab;

extern GLOBAL_DEF (symtab_t, symtab);

#define GET_SYMBOL(offset) \
  ((const char *)((GLOBAL_REF (symtab))->entry + offset))

typedef struct SymbolInternTable SymbolInternTable;
typedef struct SymbolNode SymbolNode;

struct SymbolNode
{
  RB_ENTRY (SymbolNode) entry;
  const char *str_buf;
};

static inline bool intern_symbol_compare (SymbolNode *a, SymbolNode *b)
{
  return strncmp (a->str_buf, b->str_buf, MAX_STR_LEN);
}

void make_symbol (const char *str_buf, object_t obj);
Object string_to_symbol (object_t sym);
Object symbol_to_string (object_t str);
void create_symbol_table (symtab_t st);

#endif // End of __LAMBDACHIP_SYMBOL_H__
