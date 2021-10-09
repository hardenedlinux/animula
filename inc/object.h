#ifndef __LAMBDACHIP_OBJECT_H__
#define __LAMBDACHIP_OBJECT_H__
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

#include "gc.h"
#include "os.h"
#include "qlist.h"
#include "types.h"

/*
 * NOTE:
 * Mostly, LambdaChip supports 32bit MCU, so the object uses 32bit encoding.

  Type:             0                     15         23           31
  |                 |                      |                      |
  0.  Imm Int       |              32bit signed int               |
  1.  Arbi Int      |      next cell       |      16bit int       |
  2.  Keyword       |      head pointer                           |
  3.  Pair          |      pair_t address                         |
  4.  Symbol        |      interned head pointer                  |
  5.  Vector        |      length          |      content         |
  6.  Continuation  |      parent          |      closure         |
  7.  List          |      list_t address                         |
  8.  String        |      C-string encoding                      |
  9.  Procedure     |      entry           |  arity  |   opt      |
  10. Primitive     |      primitive number                       |

  Closure on heap
                    0                                            31
  11. Closure       |      closure_t address                      |

  Closure on stack
                    0              9      15                     31
  12. Closure       |      env     | size  |      entry offset    |

  13. Real          |      IEEE 754                               |

  NOTE: The rational number is canonical-form handled by the compiler
  NOTE: We seperate +/- rational number to save RAMs
  14. +Rational     |      16bit uint      |       16bit uint     |
  15. -Rational     |      16bit uint      |       16bit uint     |

  16. Complex       |      16bit sint      |       16bit sint     |
  17. Complex       |      inexact complex address                |

  18. Mut string    |      malloced C-string header               |
  18. Mut list      |      list_t address                         |

  61. Boolean       |      false: 0,     true: 1                  |
  62. null_object   |                                             |
  63. None          |      const encoding                         |
*/

#define MAX_STR_LEN 256

extern GLOBAL_DEF (const Object, true_const);
extern GLOBAL_DEF (const Object, false_const);
extern GLOBAL_DEF (const Object, null_const);
// None object, undefined in JS, unspecified in Scheme
extern GLOBAL_DEF (const Object, none_const);

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

static inline bool is_unspecified (object_t obj)
{
  return (null_obj == obj->attr.type);
}

#define CREATE_NEW_OBJ(t, te, to)         \
  do                                      \
    {                                     \
      t o = (t)gc_pool_malloc (te);       \
      if (!o)                             \
        {                                 \
          o = (t)os_malloc (sizeof (to)); \
        }                                 \
      return o;                           \
    }                                     \
  while (0)

#define VALIDATE(obj, t)                                      \
  do                                                          \
    {                                                         \
      if ((t) != (obj)->attr.type)                            \
        {                                                     \
          PANIC ("Invalid type, expect %d, but it's %d\n", t, \
                 (obj)->attr.type);                           \
        }                                                     \
    }                                                         \
  while (0)

#define VALIDATE_STRING(obj)                                                \
  do                                                                        \
    {                                                                       \
      if ((string != (obj)->attr.type) && (mut_string != (obj)->attr.type)) \
        {                                                                   \
          PANIC ("Invalid type, expect string, but it's %d\n", t,           \
                 (obj)->attr.type);                                         \
        }                                                                   \
    }                                                                       \
  while (0)

#define VALIDATE_BYTEVECTOR(obj)                                   \
  do                                                               \
    {                                                              \
      if ((bytevector != (obj)->attr.type)                         \
          && (mut_bytevector != (obj)->attr.type))                 \
        {                                                          \
          PANIC ("Invalid type, expect bytevector, but it's %d\n", \
                 (obj)->attr.type);                                \
        }                                                          \
    }                                                              \
  while (0)

#define MAX_REAL_DENOMINATOR 0xFFFF
#define MIN_REAL_DENOMINATOR -65535
#define MAX_REAL_NUMERATOR   0xFFFF
#define MIN_REAL_NUMERATOR   -65535
#define MIN_INT32            -2147483648
#define MAX_INT32            2147483647
#define MIN_UINT8            0
#define MAX_UINT8            255
#define MIN_UINT16           0
#define MAX_UINT16           65535

#define VALIDATE_NUMERATOR(x)                                             \
  do                                                                      \
    {                                                                     \
      if (x < MIN_REAL_NUMERATOR || x > MAX_REAL_NUMERATOR)               \
        {                                                                 \
          os_printk ("%s:%d, %s: NUMERATOR not in range: %d\n", __FILE__, \
                     __LINE__, __FUNCTION__, x);                          \
          panic ("");                                                     \
        }                                                                 \
    }                                                                     \
  while (0)

#define VALIDATE_DENOMINATOR(x)                                             \
  do                                                                        \
    {                                                                       \
      if (x < MIN_REAL_DENOMINATOR || x > MAX_REAL_DENOMINATOR || x == 0)   \
        {                                                                   \
          os_printk ("%s:%d, %s: DENOMINATOR not in range: %d\n", __FILE__, \
                     __LINE__, __FUNCTION__, x);                            \
          panic ("");                                                       \
        }                                                                   \
    }                                                                       \
  while (0)

#define VALIDATE_NUMBER(obj)                                      \
  do                                                              \
    {                                                             \
      if (imm_int != (obj)->attr.type && real != (obj)->attr.type \
          && rational_pos != (obj)->attr.type                     \
          && rational_neg != (obj)->attr.type                     \
          && complex_exact != (obj)->attr.type                    \
          && complex_inexact != (obj)->attr.type)                 \
        {                                                         \
          PANIC ("Invalid type, expect numbers, but it's %d\n",   \
                 (obj)->attr.type);                               \
        }                                                         \
    }                                                             \
  while (0)

#define OBJ_IS_ON_STACK(o) ((o)->attr.gc)

closure_t make_closure (u8_t arity, u8_t frame_size, reg_t entry);
list_node_t lambdachip_new_list_node (void);
list_t lambdachip_new_list (void);
vector_t lambdachip_new_vector (void);
pair_t lambdachip_new_pair (void);
bytevector_t lambdachip_new_bytevector (void);
mut_bytevector_t lambdachip_new_mut_bytevector (void);
object_t lambdachip_new_object (otype_t type);

#endif // End of __LAMBDACHIP_OBJECT_H__
