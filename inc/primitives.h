#ifndef __ANIMULA_PRIMITIVE_H__
#define __ANIMULA_PRIMITIVE_H__
/*  Copyright (C) 2020-2025
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.

 *  Animula is distributed in the hope that it will be useful,
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

typedef float (*real_real_op_t) (float, float);
typedef s32_t (*int_int_op_t) (s32_t, s32_t);
typedef float (*real_int_op_t) (float, s32_t);
typedef rational_t (*rat_rat_op_t) (rational_t, rational_t);
typedef rational_t (*rat_int_op_t) (rational_t, s32_t);

#define NUM_OP_MAX 5
typedef enum
{
  op_add,
  op_sub,
  op_mul,
  op_div,
  op_mod
} num_op_t;

typedef enum
{
  int_int,
  real_real,
  rat_rat,
  real_int,
  rat_int
} num_op_type_t;

// NOTE: assign is not a primitive
typedef enum prim_num
{
  ret = 0,
  pop = 1,
  num_add = 2,
  num_sub = 3,
  num_mul = 4,
  fract_div = 5,
  object_print = 6,
  apply = 7,
  not = 8,
  num_eq = 9,
  num_lt = 10,
  num_gt = 11,
  num_le = 12,
  num_ge = 13,
  restore = 14,
  reserved_1 = 15,

  int_modulo = 16,
  int_remainder = 17,
  foreach = 18,
  map = 19,
  list_ref = 20,
  list_set = 21,
  list_append = 22,
  eqv = 23,
  eq = 24,
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

  // raise ; 16 + 30
  // raise-continuable ; 16 + 31
  // error ; 16 + 32
  // error-object? ; 16 + 33
  // error-object-message ; 16 + 34
  // error-object-irritants ; 16 + 35
  // read-error? ; 16 + 36
  // file-error? ; 16 + 37
  // dynamic-wind ; 16 + 38

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
  prim_floor = 80,
  prim_floor_div = 81,
  prim_ceiling = 82,
  prim_truncate = 83,
  prim_round = 84,
  prim_rationalize = 85,
  prim_floor_quotient = 86,
  prim_floor_remainder = 87,
  prim_truncate_div = 88,
  prim_truncate_quotient = 89,
  prim_truncate_remainder = 90,
  prim_numerator = 91,
  prim_denominator = 92,
  prim_is_exact_integer = 93,
  prim_is_finite = 94,
  prim_is_infinite = 95,
  prim_is_nan = 96,
  prim_is_zero = 97,
  prim_is_positive = 98,
  prim_is_negative = 99,
  prim_is_odd = 100,
  prim_is_even = 101,
  prim_square = 102,
  prim_sqrt = 103,
  prim_exact_integer_sqrt = 104,
  prim_expt = 105,
  prim_gpio_get = 106,
  prim_vm_reset = 107,
  prim_make_string = 108,
  prim_string = 109,
  prim_string_length = 110,
  prim_string_ref = 111,
  prim_string_set = 112,
  prim_string_eq = 113,
  prim_substring = 114,
  prim_string_append = 115,
  prim_string_copy = 116,
  prim_string_copy_side_effect = 117,
  prim_string_fill = 118,

  PRIM_MAX = 119,
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
#if defined ANIMULA_DEBUG
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
#if defined ANIMULA_DEBUG

  /* NOTE: The gcc8 adds a new feature to check the bound for string
   *       functions. However, strnlen is safe from the consideration,
   *       so we ignore it temporarily to make gcc happy.
   * See here for more details:
   https://stackoverflow.com/questions/50198319/gcc-8-wstringop-truncation-what-is-the-good-practice
  */
  // #  pragma GCC diagnostic push
  // #  pragma GCC diagnostic ignored "-Wstringop-overread"
  size_t len = os_strnlen (name, PRIM_NAME_SIZE);
  os_memcpy (prim->name, name, len);
  // #  pragma GCC diagnostic pop

  prim->arity = arity;
#endif
  prim->fn = fn;
  GLOBAL_REF (prim_table)[pn] = prim;
}

extern object_t _floor (vm_t vm, object_t ret, immu_object_t x);
extern object_t _floor_div (vm_t vm, object_t ret, immu_object_t x,
                            immu_object_t y);
extern object_t _ceiling (vm_t vm, object_t ret, immu_object_t x);
extern object_t _truncate (vm_t vm, object_t ret, immu_object_t x);
extern object_t _round (vm_t vm, object_t ret, immu_object_t x);
extern object_t _rationalize (vm_t vm, object_t ret, immu_object_t x);
extern object_t _floor_quotient (vm_t vm, object_t ret, immu_object_t x,
                                 immu_object_t y);
extern object_t _floor_remainder (vm_t vm, object_t ret, immu_object_t x,
                                  immu_object_t y);
extern object_t _truncate_div (vm_t vm, object_t ret, immu_object_t x,
                               immu_object_t y);
extern object_t _truncate_quotient (vm_t vm, object_t ret, immu_object_t x,
                                    immu_object_t y);
extern object_t _truncate_remainder (vm_t vm, object_t ret, immu_object_t x,
                                     immu_object_t y);
extern object_t _numerator (vm_t vm, object_t ret, immu_object_t x);
extern object_t _denominator (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_exact_integer (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_finite (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_infinite (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_nan (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_zero (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_positive (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_negative (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_odd (vm_t vm, object_t ret, immu_object_t x);
extern object_t _is_even (vm_t vm, object_t ret, immu_object_t x);
extern object_t _square (vm_t vm, object_t ret, immu_object_t x);
extern object_t _sqrt (vm_t vm, object_t ret, immu_object_t x);
extern object_t _exact_integer_sqrt (vm_t vm, object_t ret, immu_object_t x);
extern object_t _expt (vm_t vm, object_t ret, immu_object_t x, immu_object_t y);

// string
extern object_t _make_string (vm_t vm, object_t ret, immu_object_t length,
                              object_t char0);
extern object_t _string (vm_t vm, object_t ret, immu_object_t length,
                         object_t char0);
extern object_t _string_length (vm_t vm, object_t ret, immu_object_t obj);
extern object_t _string_ref (vm_t vm, object_t ret, immu_object_t obj,
                             object_t index);
extern object_t _string_set (vm_t vm, object_t ret, immu_object_t obj,
                             object_t index, immu_object_t char0);
extern object_t _string_eq (vm_t vm, object_t ret, immu_object_t str0,
                            object_t str1);
extern object_t _substring (vm_t vm, object_t ret, immu_object_t str0,
                            object_t start, immu_object_t end);
extern object_t _string_append (vm_t vm, object_t ret, immu_object_t str0,
                                object_t str1);
extern object_t _string_copy (vm_t vm, object_t ret, immu_object_t str0,
                              object_t start, immu_object_t end);
extern object_t _string_copy_side_effect (vm_t vm, object_t ret,
                                          immu_object_t str0, object_t at,
                                          immu_object_t str1, object_t start,
                                          immu_object_t end);

extern object_t _string_fill (vm_t vm, object_t ret, immu_object_t str0,
                              object_t fill, immu_object_t start,
                              immu_object_t end);

char *prim_name (u16_t pn);
void primitives_init (void);
void primitives_clean (void);
prim_t get_prim (u16_t pn);

#endif // End of __ANIMULA_PRIMITIVE_H__
