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
#include "list.h"
#include "memory.h"
#include "object.h"
#include "print.h"
#include "str.h"
#include "symbol.h"
#include "types.h"
#include "vector.h"

#define PRIM_NAME_SIZE 16
#define PRIM_MAX       32

// NOTE: assign is not a primitive
typedef enum prim_num
{
  ret = 0,
  pop = 1,
  int_add = 2,
  int_sub = 3,
  int_mul = 4,
  fract_div = 5,
  object_print = 6,
  apply = 7,
  not = 8,
  int_eq = 9,
  int_lt = 10,
  int_gt = 11,
  int_le = 12,
  int_ge = 13,
  restore = 14,
  reserved_1 = 15,

  int_modulo = 16,
  int_remainder = 17,
  foreach = 18,
  map = 19,
  list_ref = 20,
  list_set = 21,
  list_append = 22,
  eq = 23,
  eqv = 24,
  equal = 25,
  prim_usleep = 26,
  prim_gpio_pin_configure = 27,
  prim_gpio_pin_set = 28,
  prim_gpio_pin_toggle = 29,

  do_not_forget_modify_PRIM_MAX = 31
} pn_t;

typedef imm_int_t (*arith_prim_t) (imm_int_t, imm_int_t);
typedef void (*printer_prim_t) (object_t);
typedef bool (*logic_not_t) (object_t);
typedef bool (*logic_check_t) (object_t, object_t);
typedef object_t (*func_2_args_with_ret_t) (object_t, object_t);
typedef object_t (*func_3_args_with_ret_t) (object_t, object_t, object_t);

// FixMe: Shall this change to other type name?
typedef object_t (*func_1_args_with_ret_t) (object_t);

typedef struct Primitive
{
#if defined LAMBDACHIP_DEBUG
  char name[PRIM_NAME_SIZE];
  u8_t arity;
#endif
  void *fn;
} __packed *prim_t;

extern GLOBAL_DEF (prim_t, prim_table[]);

static inline void def_prim (u16_t pn, const char *name, u8_t arity, void *fn)
{
  prim_t prim = (prim_t)os_calloc (1, sizeof (struct Primitive));
#if defined LAMBDACHIP_DEBUG
  size_t len = os_strnlen (name, PRIM_NAME_SIZE);
  os_memcpy (prim->name, name, len);
  prim->arity = arity;
#endif
  prim->fn = fn;
  GLOBAL_REF (prim_table)[pn] = prim;
}

char *prim_name (u16_t pn);
void primitives_init (void);
void primitives_clean (void);
prim_t get_prim (u16_t pn);

#endif // End of __LAMBDACHIP_PRIMITIVE_H__
