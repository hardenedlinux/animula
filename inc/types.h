#ifndef __LAMBDACHIP_TYPES_H
#define __LAMBDACHIP_TYPES_H
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

#include "os.h"
#include "qlist.h"

#if defined LAMBDACHIP_ZEPHYR
#  ifndef CONFIG_HEAP_MEM_POOL_SIZE
#    error "You must define CONFIG_HEAP_MEM_POOL_SIZE for Zephyr!"
#  endif
#  include <stddef.h>
#  include <zephyr/types.h>
#  define bool _Bool
typedef u16_t reg_t;

#elif defined LAMBDACHIP_LINUX
#  define CONFIG_HEAP_MEM_POOL_SIZE 90000
#  include "__types.h"
#  include <stdint.h>

#else
#  define CONFIG_HEAP_MEM_POOL_SIZE 90000
#  include "__types.h"
#endif

#if defined __GNUC__
#  ifndef __packed
#    define __packed __attribute__ ((packed))
#  endif
#endif

typedef enum obj_encoding
{
  FALSE = 0,
  TRUE,
  GENERAL_OBJECT,
  CHAR,
  NULL_LIST,
  NONE,
  SYMBOL
} obje_t;

typedef enum obj_type
{
  imm_int = 0,
  arbi_int = 1,
  /* reserved = 2, */
  pair = 3,
  symbol = 4,
  vector = 5,
  continuation = 6,
  list = 7,
  string = 8,
  procedure = 9,
  primitive = 10,
  closure_on_heap = 11,
  closure_on_stack = 12,
  real = 13,
  rational_pos = 14,
  rational_neg = 15,
  complex_exact = 16,
  complex_inexact = 17,
  mut_string = 18,

  boolean = 61,
  null_obj = 62,
  none = 63
} otype_t;

#if defined ADDRESS_64
typedef s64_t imm_int_t;
// hov stands for half-of-value
typedef u32_t hov_t;
typedef u32_t denominator_t;
typedef u32_t numerator_t;
typedef u32_t real_part_t;
typedef u32_t imag_part_t;
#else
typedef s32_t imm_int_t;
typedef u16_t hov_t;
typedef u16_t denominator_t;
typedef u16_t numerator_t;
typedef s16_t real_part_t;
typedef s16_t imag_part_t;
#endif

typedef float f32_t;
typedef f32_t inexact_real_part_t;
typedef f32_t inexact_imag_part_t;

/* NOTE:
 * Some of the structs below will not be packed. It's because the address of the
 * field in a packed struct may cause unaligned address, which is undefined
 * activity according to C11.
 */

typedef union ObjectAttribute
{
  struct
  {
    unsigned type : 6;
    unsigned gc : 2; // for generational GC
  };
  u8_t all;
} __packed oattr;

typedef struct Procedure
{
  struct
  {
    unsigned arity : 8;
    unsigned opt : 8;
    unsigned entry : 16;
  };
  u32_t all;
} __packed Procedure, *procedure_t;

typedef struct Object
{
  oattr attr;
  union
  {
    void *value;
    Procedure proc;
  };
} __packed Object, *object_t;

typedef struct Closure
{
  oattr attr;
  u8_t arity;
  u8_t frame_size;
  reg_t entry;
  reg_t local;
  Object env[];
} Closure, *closure_t;

typedef union Continuation
{
  struct
  {
#ifndef ADDRESS_64
    unsigned parent : 16;  // the offset in ss to store parent
    unsigned closure : 16; // the offset in ss to store closure
#else
    unsigned parent : 32;
    unsigned closure : 32;
#endif
  };
  uintptr_t all;
} __packed *cont_t;

typedef SLIST_HEAD (ObjectListHead, ObjectList) obj_list_head_t;
typedef struct ObjectList
{
  SLIST_ENTRY (ObjectList) next;
  object_t obj;
} __packed ObjectList, *obj_list_t;

typedef struct List
{
  oattr attr;
  obj_list_head_t list;
} List, *list_t;

typedef struct Pair
{
  oattr attr;
  object_t car;
  object_t cdr;
} __packed Pair, *pair_t;

typedef struct Vector
{
  oattr attr;
  u16_t size;
  object_t *vec;
} __packed Vector, *vector_t;

typedef struct GCInfo
{
  u32_t fp;
  u32_t sp;
  u8_t *stack;
} __packed GCInfo, *gc_info_t;

typedef union ieee754_float
{
  float f;
  uintptr_t v;
  /* This is the IEEE 754 single-precision format.  */
  struct
  {
#if defined LAMBDACHIP_BIG_ENDIAN
    unsigned int negative : 1;
    unsigned int exponent : 8;
    unsigned int mantissa : 23;
#else
    unsigned int mantissa : 23;
    unsigned int exponent : 8;
    unsigned int negative : 1;
#endif /* Little endian.  */
  };
} real_t;

#ifndef PC_SIZE
#  define PC_SIZE 2
#  if (4 == PC_SIZE)
#    define PUSH_REG    PUSH_U32
#    define POP_REG     POP_U32
#    define NORMAL_JUMP 0xFFFFFFFF
#    define REG_BIT     32
typedef u32_t reg_t;
#  endif
#  if (2 == PC_SIZE)
#    define PUSH_REG    PUSH_U16
#    define POP_REG     POP_U16
#    define NORMAL_JUMP 0xFFFF
#    define REG_BIT     16
typedef u16_t reg_t;
#  endif
#endif

// Frame Pre-store Size = sizeof(pc) + sizeof(fp)
#define FPS 2 * PC_SIZE

static inline uintptr_t read_uintptr_from_ptr (char *ptr)
{
  u8_t buf[sizeof (uintptr_t)] = {0};
#if defined LAMBDACHIP_BIG_ENDIAN
  buf[0] = *ptr++;
  buf[1] = *ptr++;
  buf[2] = *ptr++;
  buf[3] = *ptr;
#else
  buf[3] = *ptr++;
  buf[2] = *ptr++;
  buf[1] = *ptr++;
  buf[0] = *ptr;
#endif
  return *((uintptr_t *)buf);
}
#endif // End of __LAMBDACHIP_TYPES_H;
