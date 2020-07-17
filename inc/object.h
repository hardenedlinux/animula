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

#include "types.h"
#include "qlist.h"
#include "gc.h"

/*
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
  8.  String        |      C-string encoding

  127.    N/A       |      const encoding                         |
*/

typedef enum obj_encoding
  {
   FALSE = 0, TRUE, GENERAL_OBJECT, CHAR, NULL_LIST, NONE
  } obje_t;

typedef enum obj_type
  {
   imm_int = 0, arbi_int,
   closure, pair, symbol, vector, continuation, list, string,
   special = 127
  } otype_t;

#define MAX_STR_LEN 256

extern const u8_t true_const;
extern const u8_t false_const;
extern const u8_t null_const;
extern const u8_t none_const;

typedef union ObjectAttribute
{
  struct
  {
    unsigned type: 6;
    unsigned gc: 2; // for generational GC
  };
  u8_t all;
} __packed oattr;

typedef struct Object
{
  oattr attr;
  void* value;
} __packed *object_t, Object;

typedef union Continuation
{
  struct
  {
    unsigned parent: 16; // the offset in ss to store parent
    unsigned closure: 16; // the offset in ss to store closure
  };
  u32_t all;
}__packed *cont_t;


typedef union Closure
{
  struct
  {
    unsigned env: 16; // the offset in ss to env
    unsigned entry: 16; // the offset in ss to entry
  };
  u32_t all;
}__packed *closure_t;

typedef struct Vector
{
  size_t size;
  object_t vec[];
} __packed *vec_t;

/* define Page List */
typedef SLIST_HEAD(ObjectListHead, ObjectList) obj_list_head_t;

typedef struct ObjectList
{
  SLIST_ENTRY(ObjectList) obj_list;
  object_t obj;
} __packed obj_list_t;

static inline bool object_is_false(object_t obj)
{
  panic("object_is_false hasn't been implemented yet\n");
  //return (boolean == obj->attr.type) && (obj->value == (void*)&true_const);
  return false;
}

static inline bool object_is_true(object_t obj)
{
  panic("object_is_false hasn't been implemented yet\n");
  return false;
  //return !object_is_false(obj);
}

static inline u8_t vector_ref(vec_t vec, u8_t index)
{
  /* TODO
     NOTE:
     * The return value must be u8_t which is the address in ss, otherwise we can't
     push it into the dynamic stack.
     * So the fetched value must be stored into ss.
     */

  return 0;
}

static inline void vector_set(vec_t vec, u8_t index, object_t value)
{
  // TODO
}

void init_predefined_objects(void);
void free_object(object_t obj);

#endif // End of __LAMBDACHIP_OBJECT_H__
