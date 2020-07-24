#ifndef __LAMBDACHIP_VM_H__
#define __LAMBDACHIP_VM_H__
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
#include "lef.h"
#include "memory.h"
#include "object.h"
#include "os.h"
#include "primitives.h"
#include "types.h"
#include "values.h"

typedef enum vm_state
{
  VM_RUN,
  VM_STOP,
  VM_PAUSE,
  VM_GC
} vm_state_t;

typedef struct LambdaVM
{
  u32_t pc; // program counter
  u32_t sp; // stack pointer, move when objects pushed
  u32_t fp; // last frame pointer, move when env was created
  /* NOTE:
   * The prelude would pre-execute before the actual call, so the local frame
   * was hidden by prelude, that's why we need a `local' to record the acutal
   * frame.
   */
  u32_t local; // local frame
  vm_state_t state;
  cont_t cc; // current continuation
  bytecode8_t (*fetch_next_bytecode) (struct LambdaVM *);
  u8_t *code;
  u8_t *data;
  u8_t *stack;
} __packed *vm_t;

#define FETCH_NEXT_BYTECODE() (vm->fetch_next_bytecode (vm))

#define NEXT_DATA() ((vm->fetch_next_bytecode (vm)).all)

#define VM_PANIC()                             \
  do                                           \
    {                                          \
      vm->state = VM_STOP;                     \
      os_printk ("VM: fatal error! Panic!\n"); \
    }                                          \
  while (0)

// NOTE: vm->sp always points to the first blank
#define PUSH(data)                  \
  do                                \
    {                               \
      vm->stack[vm->sp++] = (data); \
    }                               \
  while (0)

#define TOP() (vm->stack[vm->sp - 1])

#define POP() (vm->stack[--vm->sp])

#define TOPx(t, size) (*((t *)(vm->stack + vm->sp - size)))

#define POPx(t, size)             \
  ({                              \
    vm->sp -= size;               \
    *((t *)(vm->stack + vm->sp)); \
  })

#define PUSHx(t, size, data)                    \
  do                                            \
    {                                           \
      *((t *)(vm->stack + vm->sp)) = ((t)data); \
      vm->sp += size;                           \
    }                                           \
  while (0)

#define PUSH_OBJ(obj) PUSHx (Object, sizeof (Object), obj)
#define POP_OBJ()     POPx (Object, sizeof (Object))
#define TOP_OBJ()     TOPx (Object, sizeof (Object))

/* NOTE:
 * 1. frame[0] is return address, frame[1] is the last fp, so the actual offset
      begins from 2
 * 2. The type of frame[offset] is void*
 */
#define LOCAL(offset) (&((object_t) (vm->stack + vm->local))[offset])
/* #define LOCAL_CALL(offset) \ */
/*   (vm->stack + vm->stack[vm->fp + 1] + 2 + offset * sizeof (Object)) */

#define FREE_VAR(frame, offset) \
  (object_t) (&vm->stack[vm->stack[frame] + (offset) + 2])

#define PUSH_FROM_SS(bc)             \
  do                                 \
    {                                \
      u8_t i = ss_read_u8 (bc.data); \
      vm->stack[++vm->sp] = i;       \
    }                                \
  while (0)

#define HANDLE_ARITY(data)         \
  for (hov_t i = 0; i < data; i++) \
    {                              \
      PUSH (NEXT_DATA ());         \
    }

/* NOTE:
 * Jump to code[offset]
 */
#define JUMP(offset)     \
  do                     \
    {                    \
      vm->pc = (offset); \
    }                    \
  while (0)

/* NOTE:
 * The pc+1 should be store here, since it's the next instruction.
 */
#define PROC_CALL(offset)             \
  do                                  \
    {                                 \
      vm->stack[vm->fp] = vm->pc + 1; \
      vm->local = vm->fp + 2;         \
      JUMP (offset);                  \
    }                                 \
  while (0)

/* Convention:
 * 1. Save sp to fp to restore the last frame
 * 2. Save pc to stack[fp] as the return address

 stack
 +----------+---> fp
 | ret_addr |
 +----------+
 | last_fp  |
 +----------+---> local
 | local 0  |
 +----------+
 | local 1  |
 +----------+
 | local 2  |
 +----------+---> sp

*/

#define PLACEHOLD 0
/* The pc should be stored before jump, here we just fill it with a placeholder.
 */
#define SAVE_ENV()         \
  do                       \
    {                      \
      PUSH (PLACEHOLD);    \
      PUSH (vm->fp);       \
      vm->fp = vm->sp - 2; \
    }                      \
  while (0)

#define CALL_PROCEDURE(obj)                \
  do                                       \
    {                                      \
      u32_t offset = (u32_t) (obj)->value; \
      SAVE_ENV ();                         \
      PROC_CALL (offset);                  \
    }                                      \
  while (0)

#define CALL_PRIMITIVE(obj)                      \
  do                                             \
    {                                            \
      uintptr_t prim = (uintptr_t) (obj)->value; \
      call_prim (vm, prim);                      \
    }                                            \
  while (0)

#define CALL(obj)                                                    \
  do                                                                 \
    {                                                                \
      switch (obj->attr.type)                                        \
        {                                                            \
        case procedure:                                              \
          {                                                          \
            CALL_PROCEDURE (obj);                                    \
            break;                                                   \
          }                                                          \
        case primitive:                                              \
          {                                                          \
            CALL_PRIMITIVE (obj);                                    \
            break;                                                   \
          }                                                          \
        default:                                                     \
          {                                                          \
            os_printk ("Not a callable type %d!\n", obj->attr.type); \
            panic ("VM panic!");                                     \
          }                                                          \
        }                                                            \
    }                                                                \
  while (0)

static inline void call_prim (vm_t vm, pn_t pn)
{
  prim_t prim = get_prim (pn);

  switch (pn)
    {
    case ret:
      {
        // printf ("ret sp: %d, fp: %d\n", vm->sp, vm->fp);
        for (int i = 0; i < 2; i++)
          {
            /* NOTE:
             * ret is a special primitive that implies it's the tail call.
             * So we pop twice to skip its own prelude to restore the last
             * frame.
             */
            vm->sp = vm->fp + 2;
            vm->fp = POP ();
            vm->pc = POP ();
            vm->local = vm->fp + 2;
            // printf ("after sp: %d, fp: %d, pc: %d\n", vm->sp, vm->fp,
            // vm->pc);
          }
        break;
      }
    case int_add:
    case int_sub:
    case int_mul:
    case int_div:
      {
        ARITH_PRIM ();
        break;
      }
    case object_print:
      {
        printer_prim_t fn = (printer_prim_t)prim->fn;
        Object obj = POP_OBJ ();
        fn (&obj);
        PUSH_OBJ (GLOBAL_REF (none_const)); // return NONE object
        break;
      }
    default:
      os_printk ("Invalid prim number: %d\n", pn);
    }
}

static inline uintptr_t vm_get_uintptr (vm_t vm, u8_t *buf)
{
#if defined LAMBDACHIP_BIG_ENDIAN
  buf[0] = NEXT_DATA ();
  buf[1] = NEXT_DATA ();
  buf[2] = NEXT_DATA ();
  buf[3] = NEXT_DATA ();
#else
  buf[3] = NEXT_DATA ();
  buf[2] = NEXT_DATA ();
  buf[1] = NEXT_DATA ();
  buf[0] = NEXT_DATA ();
#endif
  return *((uintptr_t *)buf);
}

void vm_init (vm_t vm);
void vm_clean (vm_t vm);
void vm_restart (vm_t vm);
void vm_run (vm_t vm);
void vm_load_lef (vm_t vm, lef_t lef);
#endif // End of __LAMBDACHIP_VM_H__
