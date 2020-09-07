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

#include "closure.h"
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

#define MAX_STR_LEN 256

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
      o->attr.gc = 1;                     \
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
