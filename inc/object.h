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
 * When the platform is 64bit, then the object encoding is 64bit, however, we
 * don't bother to show it here.


  Type:             0                     15                     31
  |                 |                      |                      |
  0.  Imm Int       |              32bit signed int               |
  1.  Arbi Int      |      next cell       |      16bit int       |
  2.  Closure       |      entry offset    |      env offsert     |
  3.  Pair          |      car             |      cdr             |
  4.  Symbol        |      interned head pointer                  |
  5.  Vector        |      length          |      content         |
  6.  Continuation  |      parent          |      closure         |
  7.  List          |      length          |      content         |
  8.  String        |      C-string encoding                      |
  9.  Procedure     |      codeseg offset                         |
  10. Primitive     |      primitive number                       |
  63.    N/A       |       const encoding                         |
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
  arbi_int,
  closure,
  pair,
  symbol,
  vector,
  continuation,
  list,
  string,
  procedure,
  primitive,

  boolean = 61,
  null_obj = 62,
  none = 63
} otype_t;

#define MAX_STR_LEN 256

#if defined ADDRESS_64
typedef s32_t imm_int_t;
// hov stands for half-of-value
typedef u32_t hov_t;
#else
typedef s16_t imm_int_t;
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

typedef union Closure
{
  struct
  {
#ifndef ADDRESS_64
    unsigned env : 16;   // the offset in ss to env
    unsigned entry : 16; // the offset in ss to entry
#else
    unsigned env : 32;
    unsigned entry : 32;
#endif
  };
  uintptr_t all;
} __packed *closure_t;

typedef struct Vector
{
  size_t size;
  object_t vec[];
} __packed *vec_t;

/* define Page List */
typedef SLIST_HEAD (ObjectListHead, ObjectList) obj_list_head_t;

typedef struct ObjectList
{
  SLIST_ENTRY (ObjectList) obj_list;
  object_t obj;
} __packed obj_list_t;

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

static inline u8_t vector_ref (vec_t vec, u8_t index)
{
  /* TODO
     NOTE:
     * The return value must be u8_t which is the address in ss, otherwise we
     can't push it into the dynamic stack.
     * So the fetched value must be stored into ss.
     */

  return 0;
}

static inline void vector_set (vec_t vec, u8_t index, object_t value)
{
  // TODO
}

extern GLOBAL_DEF (const Object, true_const);
extern GLOBAL_DEF (const Object, false_const);
extern GLOBAL_DEF (const Object, null_const);
// None object, undefined in JS, unspecified in Scheme
extern GLOBAL_DEF (const Object, none_const);

void init_predefined_objects (void);
void free_object (object_t obj);

#endif // End of __LAMBDACHIP_OBJECT_H__
