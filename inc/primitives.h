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

#include "bytecode.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "types.h"

#define PRIM_NAME_SIZE 16

typedef enum prim_num
{
  ret = 0,

  int_add = 2,
  int_sub = 3,
  int_mul = 4,
  int_div = 5,
  object_print = 6,

  int_eq = 9
} pn_t;

typedef imm_int_t (*arith_prim_t) (imm_int_t, imm_int_t);
typedef void (*printer_prim_t) (object_t);

typedef struct Primitive
{
#if defined LAMBDACHIP_DEBUG
  char name[PRIM_NAME_SIZE];
#endif
  u8_t arity;
  void *fn;
} __packed *prim_t;

extern GLOBAL_DEF (prim_t, prim_table[]);

#define ARITH_PRIM()                                              \
  arith_prim_t fn = (arith_prim_t)prim->fn;                       \
  size_t size = sizeof (struct Object);                           \
  Object x = POP_OBJ ();                                          \
  Object y = POP_OBJ ();                                          \
  Object z = {.attr = {.type = imm_int, .gc = 0}, .value = NULL}; \
  z.value = (void *)fn ((imm_int_t)y.value, (imm_int_t)x.value);  \
  PUSH_OBJ (z);

#define PRIM_MAX 10

static inline void def_prim (u16_t pn, const char *name, u8_t arity, void *fn)
{
  prim_t prim = (prim_t)os_malloc (sizeof (struct Primitive));
#if defined LAMBDACHIP_DEBUG
  size_t len = os_strnlen (name, PRIM_NAME_SIZE);
  os_memcpy (prim->name, name, len);
#endif
  prim->arity = arity;
  prim->fn = fn;
  GLOBAL_REF (prim_table)[pn] = prim;
}

#if defined LAMBDACHIP_DEBUG
char *prim_name (u16_t pn);
#endif

void primitives_init (void);
void primitives_clean (void);
prim_t get_prim (u16_t pn);

#endif // End of __LAMBDACHIP_PRIMITIVE_H__
