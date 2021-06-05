#ifndef __LAMBDACHIP_VM_H__
#define __LAMBDACHIP_VM_H__
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
#define CREATE_RET_OBJ() \
  {                      \
    .attr = {.gc = 1 }   \
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
    vm->sp -= (size);             \
    *((t *)(vm->stack + vm->sp)); \
  })

#define PUSHx(t, size, data)                       \
  do                                               \
    {                                              \
      *((t *)(vm->stack + vm->sp)) = ((t) (data)); \
      vm->sp += (size);                            \
      vm_stack_check (vm);                         \
    }                                              \
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
        for (int i = 0; i < up; i++)                                           \
          {                                                                    \
            fp = *((reg_t *)(vm->stack + fp + sizeof (reg_t)));                \
          }                                                                    \
        if (false == vm->attr.tail_rec)                                        \
          closure = up ? fp_to_closure (fp) : vm->closure;                     \
        if (closure)                                                           \
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
#define GLOBAL_ASSIGN(index, var) \
  ({                              \
    (var).attr.gc = 3;            \
    vm->globals[(index)] = (var); \
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

#define PROC_CALL(offset)                                   \
  do                                                        \
    {                                                       \
      if (IS_SHADOW_FRAME ())                               \
        {                                                   \
          COPY_SHADOW_FRAME ();                             \
        }                                                   \
      else if (vm->closure && (false == vm->attr.tail_rec)) \
        {                                                   \
          save_closure (vm->fp, vm->closure);               \
        }                                                   \
      vm->closure = NULL;                                   \
      vm->local = vm->fp + FPS;                             \
      JUMP (offset);                                        \
    }                                                       \
  while (0)

/* Convention:
 * 1. Save sp to fp to restore the last frame
 * 2. Save pc to stack[fp] as the return address

 stack
 +----------+---> fp
 | ret_addr |
 +----------+
 | last_fp  |
 +----------+
 | attr     |
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
            break;                                                   \
          }                                                          \
        case TAIL_REC:                                               \
          {                                                          \
            vm->sp = vm->local + arity * sizeof (Object);            \
            vm->attr.shadow = arity;                                 \
            vm->attr.tail_rec = true;                                \
            break;                                                   \
          }                                                          \
        default:                                                     \
          {                                                          \
            reg_t sp = vm->sp;                                       \
            PUSH_REG (NORMAL_JUMP);                                  \
            PUSH_REG (sp ? (vm->fp ? vm->fp : NO_PREV_FP) : vm->fp); \
            PUSH (vm->attr.all);                                     \
            vm->attr.tail_rec = false;                               \
            vm->attr.shadow = 0;                                     \
            vm->fp = vm->sp - FPS;                                   \
            break;                                                   \
          }                                                          \
        }                                                            \
      vm_stack_check (vm);                                           \
    }                                                                \
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
            panic ("VM panic!\n");                                   \
          }                                                          \
        }                                                            \
    }                                                                \
  while (0)

typedef struct ClosureStack
{
  SLIST_ENTRY (ClosureStack) next;
  reg_t fp;
  closure_t closure;
} __packed ClosureStack, *closure_stack_t;

typedef SLIST_HEAD (ClosureStackHead, ClosureStack) closure_stack_head_t;

static closure_stack_head_t closure_stack;

static inline closure_t fp_to_closure (reg_t fp)
{
  closure_stack_t cs = NULL;
  closure_t closure = NULL;

  SLIST_FOREACH (cs, &closure_stack, next)
  {
    if (fp == cs->fp)
      {
        closure = cs->closure;
        break;
      }
  }

  return closure;
}

static inline closure_t pop_closure (vm_t vm)
{
  closure_stack_t cs = SLIST_FIRST (&closure_stack);
  closure_t closure = NULL;

  reg_t last_fp = *((reg_t *)(vm->stack + vm->fp + sizeof (reg_t)));

  if (cs && last_fp == cs->fp)
    {
      closure = cs->closure;
      SLIST_REMOVE_HEAD (&closure_stack, next);
      os_free (cs);
    }

  return closure;
}

static inline void save_closure (reg_t fp, closure_t closure)
{
  closure_stack_t cs
    = (closure_stack_t)os_malloc (sizeof (struct ClosureStack));

  cs->closure = closure;
  cs->fp = fp;
  SLIST_INSERT_HEAD (&closure_stack, cs, next);
}

/* NOTE:
 * ret is a special primitive that implies it's the tail call.
 * So we pop twice to skip its own prelude to restore the last
 * frame.
 */
#define RESTORE()                                                \
  do                                                             \
    {                                                            \
      Object ret = POP_OBJ ();                                   \
      vm->sp = vm->fp + FPS;                                     \
      vm->attr.all = POP ();                                     \
      vm->fp = POP_REG ();                                       \
      vm->fp = (NO_PREV_FP == vm->fp ? 0 : vm->fp);              \
      vm->pc = POP_REG ();                                       \
      vm->local = vm->fp + FPS;                                  \
      PUSH_OBJ (ret);                                            \
      vm->closure = vm->attr.tail_rec ? NULL : pop_closure (vm); \
    }                                                            \
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

  panic ("closure_on_stack hasn't been implemented yet!\n");
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
      /* printf ("vm->local: %d, size: %d, vm->sp: %d\n", vm->local,
       * shadow_size, */
      /*         vm->sp); */
      vm->sp -= shadow_size;
    }
  else
    {
      vm->local = vm->sp - arity * sizeof (Object);
    }

  closure->local = vm->local;

  if (false == vm->attr.tail_rec)
    {
      save_closure (vm->fp, closure);
    }
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
#endif // End of __LAMBDACHIP_VM_H__
