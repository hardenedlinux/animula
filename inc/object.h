#ifndef __LAMBDACHIP_OBJECT_H__
#define __LAMBDACHIP_OBJECT_H__
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

#include "gc.h"
#include "qlist.h"
#include "types.h"

/*
 * NOTE:
 * Mostly, LambdaChip supports 32bit MCU, so the object uses 32bit encoding.

  Type:             0                     15                     31
  |                 |                      |                      |
  0.  Imm Int       |              32bit signed int               |
  1.  Arbi Int      |      next cell       |      16bit int       |
  2.  Reserved      |                                             |
  3.  Pair          |      car             |      cdr             |
  4.  Symbol        |      interned head pointer                  |
  5.  Vector        |      length          |      content         |
  6.  Continuation  |      parent          |      closure         |
  7.  List          |      list_t address                         |
  8.  String        |      C-string encoding                      |
  9.  Procedure     |      codeseg offset                         |
  10. Primitive     |      primitive number                       |

  Closure on heap
                    0                                            31
  11. Closure       |      closure_t address                      |

  Closure on stack
                    0              9      15                     31
  12. Closure       |      env     | size  |      entry offset    |

  61. Boolean       |      false: 0,     true: 1                  |
  62. null_object   |                                             |
  63. None          |      const encoding                         |
*/

typedef enum obj_encoding
{
  FALSE = 0,
  TRUE,
  GENERAL_OBJECT,
  CHAR,
  NULL_LIST,
  NONE
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

  boolean = 61,
  null_obj = 62,
  none = 63
} otype_t;

#define MAX_STR_LEN 256

#if defined ADDRESS_64
typedef s64_t imm_int_t;
// hov stands for half-of-value
typedef u32_t hov_t;
#else
typedef s32_t imm_int_t;
typedef u16_t hov_t;
#endif

typedef union ObjectAttribute
{
  struct
  {
    unsigned type : 6;
    unsigned gc : 2; // for generational GC
  };
  u8_t all;
} __packed oattr;

typedef struct Object
{
  oattr attr;
  void *value;
} __packed *object_t, Object;

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

typedef struct Closure
{
  u8_t arity;
  u8_t frame_size;
  reg_t entry;
  Object env[];
} __packed Closure, *closure_t;

typedef SLIST_HEAD (ObjectListHead, ObjectList) obj_list_head_t;
typedef struct ObjectList
{
  SLIST_ENTRY (ObjectList) next;
  object_t obj;
} __packed ObjectList, *obj_list_t;

typedef struct List
{
  u16_t size;
  obj_list_head_t list;
} __packed List, *list_t;

typedef struct Pair
{
  object_t car;
  object_t cdr;
} __packed Pair, *pair_t;

typedef struct Vector
{
  u16_t size;
  object_t *vec;
} __packed Vector, *vector_t;

/* NOTE: All objects are stored in stack by copying, so we can't just compared
 *       the head pointer.
 */
static inline bool is_false (object_t obj)
{
  return ((boolean == obj->attr.type) && (NULL == obj->value));
}

static inline bool is_true (object_t obj)
{
  return !is_false (obj);
}

#define NEW_OBJ(t, te, to)                \
  do                                      \
    {                                     \
      t o = (t)gc_pool_malloc (te);       \
      if (!o)                             \
        {                                 \
          o = (t)gc_malloc (sizeof (to)); \
        }                                 \
      return o;                           \
    }                                     \
  while (0)

extern GLOBAL_DEF (const Object, true_const);
extern GLOBAL_DEF (const Object, false_const);
extern GLOBAL_DEF (const Object, null_const);
// None object, undefined in JS, unspecified in Scheme
extern GLOBAL_DEF (const Object, none_const);

void init_predefined_objects (void);
obj_list_t new_obj_list ();
list_t new_list ();
vector_t new_vector ();
pair_t new_pair ();
object_t new_object (u8_t type);

#endif // End of __LAMBDACHIP_OBJECT_H__
