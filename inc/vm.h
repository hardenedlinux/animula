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

extern GLOBAL_DEF (size_t, VM_CODESEG_SIZE);
extern GLOBAL_DEF (size_t, VM_DATASEG_SIZE);
extern GLOBAL_DEF (size_t, VM_STKSEG_SIZE);

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

static inline void vm_stack_check (vm_t vm)
{
  if (vm->sp >= GLOBAL_REF (VM_STKSEG_SIZE))
    panic ("Stack overflow!\n");
}

// NOTE: vm->sp always points to the first blank
#define PUSH(data)                  \
  do                                \
    {                               \
      vm->stack[vm->sp++] = (data); \
      vm_stack_check (vm);          \
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
      vm_stack_check (vm);                      \
    }                                           \
  while (0)

#define PUSH_OBJ(obj) PUSHx (Object, sizeof (Object), obj)
#define POP_OBJ()     POPx (Object, sizeof (Object))
#define TOP_OBJ()     TOPx (Object, sizeof (Object))

#define PUSH_U32(obj) PUSHx (u32_t, sizeof (u32_t), obj)
#define POP_U32()     POPx (u32_t, sizeof (u32_t))
#define TOP_U32()     TOPx (u32_t, sizeof (u32_t))

#define PUSH_U16(obj) PUSHx (u16_t, sizeof (u16_t), obj)
#define POP_U16()     POPx (u16_t, sizeof (u16_t))
#define TOP_U16()     TOPx (u16_t, sizeof (u16_t))

#ifndef PC_SIZE
#  define PC_SIZE 2
#  if (4 == PC_SIZE)
#    define PUSH_REG    PUSH_U32
#    define POP_REG     POP_U32
#    define NORMAL_JUMP 0xFFFFFFFF
#  endif
#  if (2 == PC_SIZE)
#    define PUSH_REG    PUSH_U16
#    define POP_REG     POP_U16
#    define NORMAL_JUMP 0xFFFF
#  endif
#endif

// Frame Pre-store Size = sizeof(pc) + sizeof(fp)
#define FPS 2 * PC_SIZE

/* NOTE:
 * 1. frame[0] is return address, frame[1] is the last fp, so the actual offset
      begins from 2
 * 2. The type of frame[offset] is void*
 */
#define LOCAL(offset) (&((object_t) (vm->stack + vm->local))[offset])

/* #define FREE_VAR(frame, offset) \ */
/*   (object_t) (&vm->stack[vm->stack[frame] + (offset) + FPS]) */

#define FREE_VAR(frame, offset) \
  (&((object_t) (vm->stack + vm->stack[vm->fp + 1] + FPS))[offset])

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
 * If fp is not NORMAL_JUMP, then it's tail-call or tail-recursive.
 * In this situation, we mustn't store pc.
 */
#define PROC_CALL(offset)                              \
  do                                                   \
    {                                                  \
      if (NORMAL_JUMP == ((u32_t *)vm->stack)[vm->fp]) \
        ((u32_t *)vm->stack)[vm->fp] = vm->pc;         \
      vm->local = vm->fp + FPS;                        \
      JUMP (offset);                                   \
    }                                                  \
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

/* 1. The pc should be stored before jump, here we just fill it with a
 *    placeholder.
 * 2. If bc.bc2 is 0, then it's tail call.
      If bc.bc2 is 1, then it's tail recursive.
 */
#define TAIL_CALL   0
#define TAIL_REC    1
#define NORMAL_CALL 2
#define SAVE_ENV()                  \
  do                                \
    {                               \
      switch (bc.bc2)               \
        {                           \
        case TAIL_CALL:             \
          {                         \
            break;                  \
          }                         \
        case TAIL_REC:              \
          {                         \
            vm->sp = vm->local;     \
            break;                  \
          }                         \
        default:                    \
          {                         \
            PUSH_REG (NORMAL_JUMP); \
            PUSH_REG (vm->fp);      \
            vm->fp = vm->sp - FPS;  \
            break;                  \
          }                         \
        }                           \
    }                               \
  while (0)

#define CALL_PROCEDURE(obj)                \
  do                                       \
    {                                      \
      u32_t offset = (u32_t) (obj)->value; \
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

/* NOTE:
 * ret is a special primitive that implies it's the tail call.
 * So we pop twice to skip its own prelude to restore the last
 * frame.
 */
#define RESTORE()               \
  do                            \
    {                           \
      vm->sp = vm->fp + FPS;    \
      vm->fp = POP_REG ();      \
      vm->pc = POP_REG ();      \
      vm->local = vm->fp + FPS; \
    }                           \
  while (0)

void vm_init (vm_t vm);
void vm_clean (vm_t vm);
void vm_restart (vm_t vm);
void vm_run (vm_t vm);
void vm_load_lef (vm_t vm, lef_t lef);
#endif // End of __LAMBDACHIP_VM_H__
