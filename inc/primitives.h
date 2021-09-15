#ifndef __LAMBDACHIP_PRIMITIVE_H__
#define __LAMBDACHIP_PRIMITIVE_H__
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

#include "bytecode.h"
#include "bytevector.h"
#include "debug.h"
#include "list.h"
#include "memory.h"
#include "object.h"
#include "print.h"
#include "str.h"
#include "symbol.h"
#include "types.h"
#include "vector.h"

#define PRIM_NAME_SIZE 32
#define BOARD_ID_LEN   25

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
  prim_device_configure = 27,
  prim_gpio_set = 28,
  prim_gpio_toggle = 29,
  prim_get_board_id = 30,
  cons = 31,
  car = 32,
  cdr = 33,
  read_char = 34,
  read_str = 35,
  readln = 36,
  list_to_string = 37,
  prim_i2c_read_byte = 38,
  prim_i2c_write_byte = 39,
  is_null = 40,
  is_pair = 41,
  prim_spi_transceive = 42,
  prim_i2c_read_list = 43,
  prim_i2c_write_list = 44,
  with_exception_handler = 45,
  scm_raise = 46,
  scm_raise_continuable = 47,
  is_list = 55,
  is_string = 56,
  is_char = 57,
  is_keyword = 58,
  is_symbol = 59,
  is_procedure = 60,
  is_primitive = 61,
  is_boolean = 62,
  is_number = 63,
  is_integer = 64,
  is_real = 65,
  is_complex = 66,
  is_rational = 67,
  is_exact = 68,
  is_inexact = 69,
  prim_i2c_read_bytevector = 70,
  is_bytevector = 71,
  prim_make_bytevector = 72,
  prim_bytevector_length = 73,
  prim_bytevector_u8_ref = 74,
  prim_bytevector_u8_set = 75,
  prim_bytevector_copy = 76,
  prim_bytevector_copy_overwrite = 77,
  prim_bytevector_append = 78,
  prim_i2c_write_bytevector = 79,

  PRIM_MAX = 80,
} pn_t;

#define GEN_PRIM(t)                                                  \
  {                                                                  \
    .attr = {.type = primitive, .gc = 0}, .value = (void *)((pn_t)t) \
  }

typedef imm_int_t (*arith_prim_t) (imm_int_t, imm_int_t);
typedef void (*printer_prim_t) (object_t);
typedef bool (*logic_not_t) (object_t);
typedef bool (*logic_check_t) (object_t, object_t);
typedef object_t (*func_0_args_with_ret_t) (vm_t, object_t);
typedef object_t (*func_1_args_with_ret_t) (vm_t, object_t, object_t);
typedef object_t (*func_2_args_with_ret_t) (vm_t, object_t, object_t, object_t);
typedef object_t (*func_3_args_with_ret_t) (vm_t, object_t, object_t, object_t,
                                            object_t);
typedef object_t (*func_4_args_with_ret_t) (vm_t, object_t, object_t, object_t,
                                            object_t, object_t);
typedef object_t (*func_5_args_with_ret_t) (vm_t, object_t, object_t, object_t,
                                            object_t, object_t, object_t);

typedef object_t (*func_0_args_t) (vm_t);
typedef object_t (*func_1_args_t) (vm_t, object_t);
typedef object_t (*func_2_args_t) (vm_t, object_t, object_t);
typedef object_t (*func_3_args_t) (vm_t, object_t, object_t, object_t);
typedef object_t (*func_4_args_t) (vm_t, object_t, object_t, object_t,
                                   object_t);

typedef Object (*pred_t) (object_t);

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
  if (!prim)
    {
      PANIC ("def_prim calloc failed!\n");
    }
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
