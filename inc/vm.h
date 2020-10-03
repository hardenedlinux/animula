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
#include "gc.h"
#include "lef.h"
#include "memory.h"
#include "object.h"
#include "os.h"
#include "primitives.h"
#include "symbol.h"
#include "types.h"

extern GLOBAL_DEF (size_t, VM_CODESEG_SIZE);
extern GLOBAL_DEF (size_t, VM_DATASEG_SIZE);
extern GLOBAL_DEF (size_t, VM_STKSEG_SIZE);

typedef enum vm_state
{
  VM_STOP = 0,
  VM_RUN = 1,
  VM_PAUSE = 2,
  VM_GC = 3,
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
  u8_t shadow; // shadow frame
  symtab_t symtab;
  closure_t closure; // for closure
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
  if (GLOBAL_REF (VM_STKSEG_SIZE) <= vm->sp)
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

#define TOPx(t, size)  (*((t *)(vm->stack + vm->sp - size)))
#define TOPxp(t, size) ((t *)(vm->stack + vm->sp - size))

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
#define TOP_OBJ_PTR() TOPxp (Object, sizeof (Object))

#define PUSH_U32(obj) PUSHx (u32_t, sizeof (u32_t), obj)
#define POP_U32()     POPx (u32_t, sizeof (u32_t))
#define TOP_U32()     TOPx (u32_t, sizeof (u32_t))

#define PUSH_U16(obj) PUSHx (u16_t, sizeof (u16_t), obj)
#define POP_U16()     POPx (u16_t, sizeof (u16_t))
#define TOP_U16()     TOPx (u16_t, sizeof (u16_t))

#define PUSH_CLOSURE(closure) PUSHx (Closure, sizeof (Closure), closure)
#define POP_CLOSURE()         POPx (Closure, sizeof (Closure))
#define TOP_CLOSURE()         TOPx (Closure, sizeof (Closure))
#define TOP_CLOSURE_PTR()     TOPxp (Closure, sizeof (Closure))

/* NOTE:
 * 1. frame[0] is return address, frame[1] is the last fp, so the actual offset
      begins from 2
 * 2. The type of frame[offset] is void*
 */
#define LOCAL(offset)                                                     \
  ({                                                                      \
    object_t ret = NULL;                                                  \
    if (vm->closure)                                                      \
      {                                                                   \
        if (offset >= vm->closure->frame_size)                            \
          {                                                               \
            ret = (&((object_t) (                                         \
              vm->stack + vm->local))[offset - vm->closure->frame_size]); \
          }                                                               \
        else                                                              \
          {                                                               \
            ret = (&vm->closure->env[offset]);                            \
          }                                                               \
      }                                                                   \
    else                                                                  \
      {                                                                   \
        ret = (&((object_t) (vm->stack + vm->local))[offset]);            \
      }                                                                   \
    ret;                                                                  \
  })

// LOCAL_FIX is only for debug since it doesn't print stored REG.
#define LOCAL_FIX(offset) (&((object_t) (vm->stack + vm->fp + FPS))[offset])

#define FREE_VAR(up, offset)                                        \
  ({                                                                \
    reg_t fp = vm->fp;                                              \
    for (int i = 0; i < up; i++)                                    \
      {                                                             \
        fp = *((reg_t *)(vm->stack + fp + sizeof (reg_t)));         \
      }                                                             \
    (object_t) (vm->stack + fp + (offset) * sizeof (Object) + FPS); \
  })

/* #define FREE_VAR(frame, offset) \ */
/*   (&((object_t) (vm->stack + vm->stack[vm->fp + 1] + FPS))[offset]) */

#define PUSH_FROM_SS(bc)             \
  do                                 \
    {                                \
      u8_t i = ss_read_u8 (bc.data); \
      vm->stack[++vm->sp] = i;       \
    }                                \
  while (0)

/* NOTE:
 * Shadow frame is used for tail-recursive for passing args correctly.
 */
#define IS_SHADOW_FRAME() (vm->shadow && vm->sp != vm->local)
#define COPY_SHADOW_FRAME()                                               \
  do                                                                      \
    {                                                                     \
      size_t size = sizeof (Object) * vm->shadow;                         \
      os_memcpy (vm->stack + vm->local, vm->stack + vm->sp - size, size); \
      vm->shadow = 0;                                                     \
      vm->sp = vm->local + size;                                          \
    }                                                                     \
  while (0)

/* NOTE:
 * Jump to code[offset]
 */
#define JUMP(offset)     \
  do                     \
    {                    \
      vm->pc = (offset); \
    }                    \
  while (0)

#define PROC_CALL(offset)       \
  do                            \
    {                           \
      if (IS_SHADOW_FRAME ())   \
        COPY_SHADOW_FRAME ();   \
      vm->local = vm->fp + FPS; \
      JUMP (offset);            \
    }                           \
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
#define TAIL_CALL     0
#define TAIL_REC      1
#define NORMAL_CALL   2
#define PROC_ARITY(b) (((b)&0xFC) >> 2)
#define PROC_MODE(b)  ((b)&0x3)
#define SAVE_ENV()                             \
  do                                           \
    {                                          \
      u8_t arity = PROC_ARITY (bc.bc2);        \
      u8_t mode = PROC_MODE (bc.bc2);          \
      switch (mode)                            \
        {                                      \
        case TAIL_CALL:                        \
          {                                    \
            break;                             \
          }                                    \
        case TAIL_REC:                         \
          {                                    \
            vm->shadow = arity;                \
            vm->sp += sizeof (Object) * arity; \
            break;                             \
          }                                    \
        default:                               \
          {                                    \
            PUSH_REG (NORMAL_JUMP);            \
            PUSH_REG (vm->fp);                 \
            vm->fp = vm->sp - FPS;             \
            break;                             \
          }                                    \
        }                                      \
      vm_stack_check (vm);                     \
    }                                          \
  while (0)

#define FIX_PC()                                           \
  do                                                       \
    {                                                      \
      if (NORMAL_JUMP == *((reg_t *)(vm->stack + vm->fp))) \
        *((reg_t *)(vm->stack + vm->fp)) = (reg_t)vm->pc;  \
    }                                                      \
  while (0)

/* NOTE:
 * If fp is not NORMAL_JUMP, then it's tail-call or tail-recursive.
 * In this situation, we mustn't store pc.
 */
#define CALL(obj)                                                    \
  do                                                                 \
    {                                                                \
      FIX_PC ();                                                     \
      switch (obj->attr.type)                                        \
        {                                                            \
        case procedure:                                              \
          {                                                          \
            VM_DEBUG ("call proc!\n");                               \
            CALL_PROCEDURE (obj);                                    \
            break;                                                   \
          }                                                          \
        case primitive:                                              \
          {                                                          \
            VM_DEBUG ("call prim!\n");                               \
            CALL_PRIMITIVE (obj);                                    \
            break;                                                   \
          }                                                          \
        case closure_on_stack:                                       \
          {                                                          \
            call_closure_on_stack (vm, obj);                         \
            break;                                                   \
          }                                                          \
        case closure_on_heap:                                        \
          {                                                          \
            VM_DEBUG ("call closure!\n");                            \
            call_closure_on_heap (vm, obj);                          \
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
  for (int i = 0; i < 2; i++)   \
    {                           \
      vm->sp = vm->fp + FPS;    \
      vm->fp = POP_REG ();      \
      vm->pc = POP_REG ();      \
      vm->local = vm->fp + FPS; \
      if (vm->closure)          \
        vm->closure = NULL;     \
    }                           \
  while (0)

#define CALL_PROCEDURE(obj)           \
  do                                  \
    {                                 \
      u16_t offset = obj->proc.entry; \
      PROC_CALL (offset);             \
    }                                 \
  while (0)

#define CALL_PRIMITIVE(obj)                      \
  do                                             \
    {                                            \
      uintptr_t prim = (uintptr_t) (obj)->value; \
      call_prim (vm, prim);                      \
    }                                            \
  while (0)

#define GC()                                                         \
  do                                                                 \
    {                                                                \
      printf ("oh GC?!\n");                                          \
      GCInfo gci = {.fp = vm->fp, .sp = vm->sp, .stack = vm->stack}; \
      gc (&gci);                                                     \
    }                                                                \
  while (0)

#define GC_MALLOC(size)                 \
  ({                                    \
    void *ret = NULL;                   \
    do                                  \
      {                                 \
        ret = (void *)os_malloc (size); \
        if (ret)                        \
          break;                        \
        GC ();                          \
      }                                 \
    while (1);                          \
    ret;                                \
  })

#define NEW_OBJ(type)                       \
  ({                                        \
    object_t obj = NULL;                    \
    do                                      \
      {                                     \
        obj = lambdachip_new_object (type); \
        if (obj)                            \
          break;                            \
        GC ();                              \
      }                                     \
    while (1);                              \
    obj;                                    \
  })

#define NEW(type)                       \
  ({                                    \
    type##_t obj = NULL;                \
    do                                  \
      {                                 \
        obj = lambdachip_new_##type (); \
        if (obj)                        \
          break;                        \
        GC ();                          \
      }                                 \
    while (1);                          \
    obj;                                \
  })

#define NEED_VARGS(p) \
  ((procedure == (p)->attr.type) && ((p)->proc.arity != (p)->proc.opt))

#define COUNT_ARGS() (vm->sp - vm->fp + FPS) / sizeof (Object)

#define IS_PROC_END(bc) \
  (IS_SPECIAL (bc) && (PRIMITIVE == (bc).type) && (restore == (bc).data))

static inline void call_closure_on_stack (vm_t vm, object_t obj)
{
  uintptr_t data = (uintptr_t) (obj)->value;
  reg_t env = (0x3FF & data);
  u8_t size = (0xFC00 & data);
  reg_t entry = ((0xFFFF0000 & data) >> 16);
  reg_t total_size = size * sizeof (Object);
  VM_DEBUG ("(closure-on-stack %d %d 0x%x)\n", size, env, entry);
  memcpy ((char *)(vm->stack + vm->sp), (char *)(vm->stack + env), total_size);
  vm->sp += total_size;
  JUMP (entry);
}

static inline void call_closure_on_heap (vm_t vm, object_t obj)
{
  closure_t closure = (closure_t) (obj)->value;
  u8_t size = closure->frame_size;
  object_t env = closure->env;
  reg_t entry = closure->entry;
  u8_t arity = closure->arity;
  reg_t total_size = size * sizeof (Object);
  VM_DEBUG ("(closure-on-heap %d %d %p 0x%x)\n", arity, size, env, entry);
  vm->closure = closure;
  vm->local = vm->sp - arity * sizeof (Object);
  vm->sp += total_size;
  JUMP (entry);
}

void vm_init (vm_t vm);
void vm_clean (vm_t vm);
void vm_restart (vm_t vm);
void vm_run (vm_t vm);
void vm_load_lef (vm_t vm, lef_t lef);
void apply_proc (vm_t vm, object_t proc, object_t ret);
#endif // End of __LAMBDACHIP_VM_H__
