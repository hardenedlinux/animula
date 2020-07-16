#ifndef __LAMBDACHIP_PRIMITIVE_H__
#define __LAMBDACHIP_PRIMITIVE_H__
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

#include "types.h"
#include "memory.h"
#include "debug.h"
#include "bytecode.h"
#include "object.h"

#define PRIM_NAME_SIZE 16

typedef enum prim_num
  {
   int_add = 2, int_sub, int_mul, int_div, object_print
  } pn_t;

typedef u8_t (*arith_prim_t)(u8_t, u8_t);
typedef void (*printer_prim_t)(object_t);

typedef struct Primitive
{
#if defined LAMBDACHIP_DEBUG
  char name[PRIM_NAME_SIZE];
#endif
  u8_t arity;
  void* fn;
} __packed *prim_t;

extern prim_t __prim_table[];

#define ARITH_PRIM()                            \
  arith_prim_t fn = (arith_prim_t)prim->fn;     \
  object_t x = (object_t)POP_ADDR();		\
  object_t y = (object_t)POP_ADDR();		\
  PUSH(fn((s32_t)x->value, (s32_t)y->value));

#define PRIM_MAX 10

static inline void def_prim(u16_t pn, const char* name, u8_t arity, void* fn)
{
  prim_t prim = (prim_t)os_malloc(sizeof(struct Primitive));
#if defined LAMBDACHIP_DEBUG
  size_t len = os_strnlen(name, PRIM_NAME_SIZE);
  os_memcpy(prim->name, name, len);
#endif
  prim->arity = arity;
  prim->fn = fn;
  __prim_table[pn] = prim;
}

#if defined LAMBDACHIP_DEBUG
char* prim_name(u16_t pn);
#endif

void primitives_init(void);
prim_t get_prim(u16_t pn);

#endif // End of __LAMBDACHIP_PRIMITIVE_H__
