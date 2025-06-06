#ifndef __ANIMULA_VM_H__
#define __ANIMULA_VM_H__
/*  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.

 *  Animula is distributed in the hope that it will be useful,
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
#include "lib.h"
#include "memory.h"
#include "object.h"
#include "os.h"
#include "primitives.h"
#include "symbol.h"
#include "types.h"

extern GLOBAL_DEF (size_t, VM_CODESEG_SIZE);
extern GLOBAL_DEF (size_t, VM_DATASEG_SIZE);
extern GLOBAL_DEF (size_t, VM_STKSEG_SIZE);

/* NOTE:
 * The magic here is to set gc=1. Although ret_obj is on the stack, this magic
 * can prevent GC mis-recycle the sub-object of ret_obj.
 */
#define CREATE_RET_OBJ()       \
  {                            \
    .attr = {.gc = GEN_1_OBJ } \
  }

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
    PANIC ("Stack overflow!\n");
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

#define TOPx(t, size)             (*((t *)(vm->stack + vm->sp - size)))
#define TOPxp(t, size)            ((t *)(vm->stack + vm->sp - size))
#define TOP_FROMxp(from, t, size) ((t *)(vm->stack + vm->sp + (from)-size))

#define POPx(t, size)             \
  ({                              \
    vm->sp -= (size);             \
    *((t *)(vm->stack + vm->sp)); \
  })

#define POP_FROMx(from, t, size)  \
  ({                              \
    (from) -= (size);             \
    *((t *)(vm->stack + (from))); \
  })

#define PUSHx(t, size, data)                       \
  do                                               \
    {                                              \
      *((t *)(vm->stack + vm->sp)) = ((t) (data)); \
      vm->sp += (size);                            \
      vm_stack_check (vm);                         \
    }                                              \
  while (0)

#define PUSH_FROMx(from, t, size, data)                     \
  do                                                        \
    {                                                       \
      *((t *)(vm->stack + (from) + vm->sp)) = ((t) (data)); \
      vm->sp += (size);                                     \
      vm_stack_check (vm);                                  \
    }                                                       \
  while (0)

#define PUSH_OBJ(obj)            PUSHx (Object, sizeof (Object), obj)
#define PUSH_OBJ_FROM(obj, from) PUSH_FROMx (from, Object, sizeof (Object), obj)
#define POP_OBJ()                POPx (Object, sizeof (Object))
#define POP_OBJ_FROM(from)       POP_FROMx (from, Object, sizeof (Object))
#define TOP_OBJ_PTR_FROM(from)   TOP_FROMxp (from, Object, sizeof (Object))
#define TOP_OBJ()                TOPx (Object, sizeof (Object))
#define TOP_OBJ_PTR()            TOPxp (Object, sizeof (Object))

#define PUSH_U32(obj) PUSHx (u32_t, sizeof (u32_t), obj)
#define POP_U32()     POPx (u32_t, sizeof (u32_t))
#define TOP_U32()     TOPx (u32_t, sizeof (u32_t))

#define PUSH_U64(obj) PUSHx (u64_t, sizeof (u64_t), obj)
#define POP_U64()     POPx (u64_t, sizeof (u64_t))
#define TOP_U64()     TOPx (u64_t, sizeof (u64_t))

#define PUSH_U16(obj) PUSHx (u16_t, sizeof (u16_t), obj)
#define POP_U16()     POPx (u16_t, sizeof (u16_t))
#define TOP_U16()     TOPx (u16_t, sizeof (u16_t))

#define PUSH_CLOSURE(obj) PUSHx (closure_t, sizeof (closure_t), obj)
#define POP_CLOSURE()     POPx (closure_t, sizeof (closure_t))
#define TOP_CLOSURE()     TOPx (closure_t, sizeof (closure_t))

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

/* NOTE:
 * Because vm->local is activate iff the actual calling occurs, so (free 0 n)
 * will refer the current frame. And because the offset of free-var doesn't
 * require fix, so we're not going to call LOCAL, but use the raw reference from
 * vm->local.
 */
#define FREE_VAR(up, offset)                                                   \
  ({                                                                           \
    reg_t fp = vm->fp;                                                         \
    object_t ret = NULL;                                                       \
    closure_t closure = NULL;                                                  \
    if (0 == up)                                                               \
      {                                                                        \
        ret = LOCAL (offset);                                                  \
      }                                                                        \
    else                                                                       \
      {                                                                        \
        for (int i = 0; i <= up; i++)                                          \
          {                                                                    \
            fp = *((reg_t *)(vm->stack + fp + sizeof (reg_t)));                \
          }                                                                    \
        closure = *((closure_t *)(vm->stack + fp + FPS - sizeof (closure_t))); \
        if (closure && closure->frame_size)                                    \
          {                                                                    \
            if (offset >= closure->frame_size)                                 \
              {                                                                \
                ret = (&((object_t) (                                          \
                  vm->stack + closure->local))[offset - closure->frame_size]); \
              }                                                                \
            else                                                               \
              {                                                                \
                ret = (&closure->env[offset]);                                 \
              }                                                                \
          }                                                                    \
        else                                                                   \
          {                                                                    \
            reg_t local = (NO_PREV_FP == fp ? 0 : fp + FPS);                   \
            ret = (&((object_t) (vm->stack + local))[offset]);                 \
          }                                                                    \
      }                                                                        \
    ret;                                                                       \
  })

#define GLOBAL(index) (vm->globals[(index)])
#define GLOBAL_ASSIGN(index, var)  \
  ({                               \
    (var).attr.gc = PERMANENT_OBJ; \
    vm->globals[(index)] = (var);  \
  })

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
#define IS_SHADOW_FRAME() (vm->attr.shadow && (vm->sp != vm->local))
#define COPY_SHADOW_FRAME()                                               \
  do                                                                      \
    {                                                                     \
      size_t size = sizeof (Object) * vm->attr.shadow;                    \
      gc_recycle_current_frame (vm->stack, vm->fp + FPS, vm->sp - size);  \
      os_memcpy (vm->stack + vm->local, vm->stack + vm->sp - size, size); \
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
        {                       \
          COPY_SHADOW_FRAME (); \
        }                       \
      vm->closure = NULL;       \
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
 |  local   |
 |----------|
 | last_fp  |
 +----------+
 |  attr    |
 +----------+
 | closure  |
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
 */
#define TAIL_CALL     0
#define TAIL_REC      1
#define NORMAL_CALL   2
#define PROC_ARITY(b) (((b)&0xFC) >> 2)
#define PROC_MODE(b)  ((b)&0x3)
#define SAVE_ENV()                                                   \
  do                                                                 \
    {                                                                \
      u8_t arity = PROC_ARITY (bc.bc2);                              \
      u8_t mode = PROC_MODE (bc.bc2);                                \
      switch (mode)                                                  \
        {                                                            \
        case TAIL_CALL:                                              \
          {                                                          \
            vm->attr.mode = TAIL_CALL;                               \
            break;                                                   \
          }                                                          \
        case TAIL_REC:                                               \
          {                                                          \
            vm->sp = vm->local + arity * sizeof (Object);            \
            vm->attr.shadow = arity;                                 \
            vm->attr.mode = TAIL_REC;                                \
            break;                                                   \
          }                                                          \
        default:                                                     \
          {                                                          \
            reg_t sp = vm->sp;                                       \
            PUSH_REG (NORMAL_JUMP);                                  \
            PUSH_REG (vm->local);                                    \
            PUSH_REG (sp ? (vm->fp ? vm->fp : NO_PREV_FP) : vm->fp); \
            PUSH (vm->attr.all);                                     \
            PUSH_CLOSURE (vm->closure);                              \
            vm->attr.shadow = 0;                                     \
            vm->fp = vm->sp - FPS;                                   \
            vm->attr.mode = NORMAL_CALL;                             \
            break;                                                   \
          }                                                          \
        }                                                            \
      vm_stack_check (vm);                                           \
    }                                                                \
  while (0)

#define SAVE_ENV_SIMPLE()         \
  do                              \
    {                             \
      PUSH_REG (vm->pc);          \
      PUSH_REG (vm->local);       \
      PUSH_REG (vm->fp);          \
      PUSH (vm->attr.all);        \
      PUSH_CLOSURE (vm->closure); \
      vm->fp = vm->sp - FPS;      \
      vm->local = vm->sp;         \
    }                             \
  while (0)

#define RESTORE_SIMPLE()            \
  do                                \
    {                               \
      Object ret_obj = POP_OBJ ();  \
      vm->sp = vm->fp + FPS;        \
      vm->closure = POP_CLOSURE (); \
      vm->attr.all = POP ();        \
      vm->fp = POP_REG ();          \
      vm->local = POP_REG ();       \
      vm->pc = POP_REG ();          \
      PUSH_OBJ (ret_obj);           \
    }                               \
  while (0)

#define FIX_PC()                                           \
  do                                                       \
    {                                                      \
      if (NORMAL_JUMP == *((reg_t *)(vm->stack + vm->fp))) \
        *((reg_t *)(vm->stack + vm->fp)) = (reg_t)vm->pc;  \
    }                                                      \
  while (0)

#define IS_PRIM(obj, prim) \
  ((primitive == (obj)->attr.type) && prim == (uintptr_t) (obj)->value)

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
            VM_DEBUG ("call prim %d!\n", (int)obj->value);           \
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
            PANIC ("VM panic!\n");                                   \
          }                                                          \
        }                                                            \
    }                                                                \
  while (0)

/* NOTE:
 * ret is a special primitive that implies it's the tail call.
 * So we pop twice to skip its own prelude to restore the last
 * frame.
 */
//      gc_recycle_current_frame (vm->stack, vm->fp + FPS, vm->sp);
#define RESTORE()                                                 \
  do                                                              \
    {                                                             \
      Object ret_obj = POP_OBJ ();                                \
      gc_recycle_current_frame (vm->stack, vm->fp + FPS, vm->sp); \
      vm->sp = vm->fp + FPS;                                      \
      vm->closure = POP_CLOSURE ();                               \
      vm->attr.all = POP ();                                      \
      vm->fp = POP_REG ();                                        \
      vm->fp = (NO_PREV_FP == vm->fp ? 0 : vm->fp);               \
      vm->local = POP_REG ();                                     \
      vm->pc = POP_REG ();                                        \
      PUSH_OBJ (ret_obj);                                         \
    }                                                             \
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

#define NEED_VARGS(p) \
  ((procedure == (p)->attr.type) && ((p)->proc.arity != (p)->proc.opt))

#define COUNT_ARGS() (vm->sp - vm->fp + FPS) / sizeof (Object)

#define IS_PROC_END(bc) \
  (IS_SPECIAL (bc) && (PRIMITIVE == (bc).type) && (restore == (bc).data))

static inline void call_closure_on_stack (vm_t vm, object_t obj)
{
  /* uintptr_t data = (uintptr_t) (obj)->value; */
  /* reg_t env = (0x3FF & data); */
  /* u8_t size = (0xFC00 & data); */
  /* reg_t entry = ((0xFFFF0000 & data) >> 16); */
  /* reg_t total_size = size * sizeof (Object); */
  /* VM_DEBUG ("(closure-on-stack %d %d 0x%x)\n", size, env, entry); */
  /* memcpy ((char *)(vm->stack + vm->sp), (char *)(vm->stack + env),
   * total_size); */
  /* vm->sp += total_size; */
  /* JUMP (entry); */

  PANIC ("closure_on_stack hasn't been implemented yet!\n");
}

static inline void call_closure_on_heap (vm_t vm, object_t obj)
{
  closure_t closure = (closure_t) (obj)->value;
  u8_t size = closure->frame_size;
  object_t env = closure->env;
  reg_t entry = closure->entry;
  u8_t arity = closure->arity;
  VM_DEBUG ("(closure-on-heap %d %d %p 0x%x)\n", arity, size, env, entry);
  /* NOTE:
   * If we call a closure inside a closure, we have to save the current
   * closure, otherwise we lose the information to reference free vars of
   * the closure.
   */
  if (IS_SHADOW_FRAME ())
    {
      size_t shadow_size = sizeof (Object) * vm->attr.shadow;
      // Copy shadow frame of closure
      os_memcpy (vm->stack + vm->local, vm->stack + vm->sp - shadow_size,
                 shadow_size);
      /* os_printk ("vm->local: %d, size: %d, vm->sp: %d\n", vm->local,
       * shadow_size, */
      /*         vm->sp); */
      vm->sp -= shadow_size;
    }
  else
    {
      vm->local = vm->sp - arity * sizeof (Object);
    }

  closure->local = vm->local;
  vm->closure = closure;
  JUMP (entry);
}

void vm_init (vm_t vm);
void vm_init_environment (vm_t vm);
void vm_clean (vm_t vm);
void vm_restart (vm_t vm);
void vm_run (vm_t vm);
void vm_load_lef (vm_t vm, lef_t lef);
void apply_proc (vm_t vm, object_t proc, object_t ret);
void call_prim (vm_t vm, pn_t pn);
#endif // End of __ANIMULA_VM_H__
