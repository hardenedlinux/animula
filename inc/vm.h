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
  VM_GC,
  VM_POP
} vm_state_t;

typedef enum vm_op
{
  VM_PUSH = 0,
  VM_POP
} vm_op_t;

typedef struct LambdaVM
{
  u32_t pc; // program counter
  u32_t sp; // stack pointer, move when objects pushed
  u32_t fp; // last frame pointer, move when env was created
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

#define PUSH(data)                  \
  do                                \
    {                               \
      vm->stack[++vm->sp] = (data); \
    }                               \
  while (0)

#define TOP() (vm->stack[vm->sp])

#define POP() (vm->stack[vm->sp--])

#define TOPx(t) (((t *)vm->stack)[vm->sp])

#define POPx(t) (((t *)vm->stack)[vm->sp--])

#define PUSHx(t, data)                        \
  do                                          \
    {                                         \
      ((t *)vm->stack)[++vm->sp] = ((t)data); \
    }                                         \
  while (0)

/* NOTE:
 * 1. frame[0] is return address, so the offset begins from 1
 * 2. The type of frame[offset] is void*
 */
#define LOCAL(offset)           (vm->stack[vm->fp + (offset) + 1])
#define FREE_VAR(frame, offset) (vm->stack[vm->stack[frame] + (offset) + 1])

#define PUSH_FROM_SS(bc)             \
  do                                 \
    {                                \
      u8_t i = ss_read_u8 (bc.data); \
      vm->stack[++vm->sp] = i;       \
    }                                \
  while (0)

#define HANDLE_ARITY(data)       \
  for (int i = 0; i < data; i++) \
    {                            \
      PUSH (NEXT_DATA ());       \
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

/* Convention:
 * 1. Save sp to fp to restore the last frame
 * 2. Save pc to stack[fp] as the return address
 */
#define SAVE_ENV()                \
  do                              \
    {                             \
      vm->fp = vm->sp;            \
      vm->stack[vm->fp] = vm->pc; \
    }                             \
  while (0)

void vm_init (vm_t vm);
void vm_clean (vm_t vm);
void vm_restart (vm_t vm);
void vm_run (vm_t vm);
void vm_load_lef (vm_t vm, lef_t lef);
#endif // End of __LAMBDACHIP_VM_H__
