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

#include "vm.h"

static void handle_optional_args (vm_t vm, object_t proc)
{
  u8_t cnt = COUNT_ARGS () - proc->proc.opt;
  Object varg = {.attr = {.type = list, .gc = 0}, .value = (void *)NEW (list)};
  obj_list_head_t *head = LIST_OBJECT_HEAD (&varg);

  for (int i = 0; i < cnt; i++)
    {
      object_t new_obj = NEW_OBJ (0);
      *new_obj = POP_OBJ ();
      new_obj->attr.gc = 1; // don't forget
      obj_list_t bl = (obj_list_t)GC_MALLOC (sizeof (ObjectList));
      bl->obj = new_obj;
      SLIST_INSERT_HEAD (head, bl, next);
    }

  PUSH_OBJ (varg);
}

static closure_t create_closure (vm_t vm, u8_t arity, u8_t frame_size,
                                 reg_t entry)
{
  closure_t closure = make_closure (arity, frame_size, entry);

  for (u8_t i = frame_size; i > 0; i--)
    {
      closure->env[i - 1] = POP_OBJ ();
      /* os_printk ("capture local-%d ", i - 1); */
      /* object_printer (&closure->env[i - 1]); */
      /* os_printk ("\n"); */
    }

  return closure;
}

static void call_prim (vm_t vm, pn_t pn)
{
  prim_t prim = get_prim (pn);

  switch (pn)
    {
    case ret:
      {
        break;
      }
    case restore:
      {
        RESTORE ();
        break;
      }
    case fract_div:
      {
        Object d = POP_OBJ ();
        Object n = POP_OBJ ();
        // TODO: rationalize the result
        imm_int_t g = gcd ((imm_int_t)d.value, (imm_int_t)n.value);
        imm_int_t dd = (imm_int_t)d.value / g;
        imm_int_t nn = (imm_int_t)n.value / g;
        uintptr_t v = ((nn << 16) | dd);
        int t = (dd ^ nn) < 0 ? rational_neg : rational_pos;
        Object ret = {.attr = {.type = t, .gc = 0}, .value = (void *)v};
        PUSH_OBJ (ret);
        break;
      }
    case int_add:
    case int_sub:
    case int_mul:
    case int_modulo:
    case int_remainder:
      {
        arith_prim_t fn = (arith_prim_t)prim->fn;
        size_t size = sizeof (struct Object);
        Object xsor = POP_OBJ ();
        Object xend = POP_OBJ ();
        Object ret = {.attr = {.type = imm_int, .gc = 0}, .value = NULL};
        VALIDATE (&xsor, imm_int);
        VALIDATE (&xend, imm_int);
        ret.value = (void *)fn ((imm_int_t)xend.value, (imm_int_t)xsor.value);
        PUSH_OBJ (ret);
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
    case int_eq:
    case int_lt:
    case int_gt:
    case int_le:
    case int_ge:
    case eq:
    case eqv:
    case equal:
      {
        logic_check_t fn = (logic_check_t)prim->fn;
        Object comparee = POP_OBJ ();
        Object comparand = POP_OBJ ();
        if (fn (&comparand, &comparee))
          PUSH_OBJ (GLOBAL_REF (true_const));
        else
          PUSH_OBJ (GLOBAL_REF (false_const));
        break;
      }
    case not:
      {
        logic_not_t fn = (logic_not_t)prim->fn;
        Object obj = POP_OBJ ();
        if (fn (&obj))
          PUSH_OBJ (GLOBAL_REF (true_const));
        else
          PUSH_OBJ (GLOBAL_REF (false_const));
        break;
      }
    case pop:
      {
        if (vm->sp)
          {
            POP_OBJ ();
          }
        break;
      }
    case map:
      {
        Object lst = POP_OBJ ();
        Object proc = POP_OBJ ();
        obj_list_head_t *head = LIST_OBJECT_HEAD (&lst);
        obj_list_t node = NULL;
        obj_list_t prev = NULL;
        /* We always set k as return */
        Object k = GEN_PRIM (ret);
        list_t new_list = NEW (list);
        Object new_list_obj
          = {.attr = {.type = list, .gc = 0}, .value = (void *)new_list};
        obj_list_head_t *new_head = LIST_OBJECT_HEAD (&new_list_obj);

        PUSH_REG (vm->pc);
        PUSH_REG (vm->fp);
        vm->fp = vm->sp - FPS;
        vm->local = vm->sp;
        SLIST_FOREACH (node, head, next)
        {
          object_t ret = NEW_OBJ (0);
          obj_list_t new_node = new_obj_list ();
          new_node->obj = ret;
          vm->sp = vm->local;
          PUSH_OBJ (k);
          PUSH_OBJ (*node->obj);

          switch (proc.attr.type)
            {
            case procedure:
              {
                apply_proc (vm, &proc, ret);
                break;
              }
            case primitive:
              {
                call_prim (vm, (pn_t)proc.value);
                *ret = POP_OBJ ();
                break;
              }
            }

          if (!prev)
            {
              // when the new list is still empty
              SLIST_INSERT_HEAD (new_head, new_node, next);
            }
          else
            {
              SLIST_INSERT_AFTER (prev, new_node, next);
            }

          prev = new_node;
          /* NOTE:
           * We're not going to create frame for each proc call in order
           * to make it faster. So we have to drop the dirty frame by resetting
           * sp to local each round.
           * So does for-each
           */
          vm->sp = vm->local;
        }

        RESTORE ();
        PUSH_OBJ (new_list_obj);
        break;
      }
    case foreach:
      {
        /* We always set k as return */
        Object k = GEN_PRIM (ret);
        Object lst = POP_OBJ ();
        Object proc = POP_OBJ ();
        obj_list_head_t *head = LIST_OBJECT_HEAD (&lst);
        obj_list_t node = NULL;

        PUSH_REG (vm->pc);
        PUSH_REG (vm->fp);
        vm->fp = vm->sp - FPS;
        vm->local = vm->fp + FPS;
        SLIST_FOREACH (node, head, next)
        {
          // TODO: support for-each in multiple lists
          vm->sp = vm->local;
          PUSH_OBJ (k);
          PUSH_OBJ (*(node->obj));
          switch (proc.attr.type)
            {
            case procedure:
              {
                apply_proc (vm, &proc, ret);
                break;
              }
            case primitive:
              {
                call_prim (vm, (pn_t)proc.value);
                break;
              }
            }
          vm->sp = vm->local;
        }

        RESTORE ();
        PUSH_OBJ (GLOBAL_REF (none_const)); // return NONE object
        break;
      }
    case apply:
      {
        VM_DEBUG ("(call apply)\n");
        Object args = POP_OBJ ();
        Object proc = POP_OBJ ();
        Object ret = {0};
        obj_list_head_t *head = LIST_OBJECT_HEAD (&args);
        obj_list_t node = NULL;

        SLIST_FOREACH (node, head, next)
        {
          PUSH_OBJ (*node->obj);
        }

        FIX_PC ();

        switch (proc.attr.type)
          {
          case procedure:
            {
              VM_DEBUG ("apply proc\n");
              // vm->local = vm->fp + FPS;
              apply_proc (vm, &proc, &ret);
              PUSH_OBJ (ret);
              break;
            }
          case primitive:
            {
              VM_DEBUG ("apply prim %d\n", (pn_t)proc.value);
              call_prim (vm, (pn_t)proc.value);
              break;
            }
          case closure_on_heap:
            {
              VM_DEBUG ("apply closure %x\n", ((closure_t)proc.value)->entry);
              call_closure_on_heap (vm, &proc);
              break;
            }
          default:
            {
              os_printk ("apply: not an applicable object, type: %d\n",
                         proc.attr.type);
              panic ("apply panic!\n");
            }
          }
        break;
      }
    case list_append:
    case list_ref:
      {
        func_2_args_with_ret_t fn = (func_2_args_with_ret_t)prim->fn;
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        PUSH_OBJ (*fn (&o1, &o2));
        break;
      }
    case prim_usleep:
      {
        func_1_args_with_ret_t fn = (func_1_args_with_ret_t)prim->fn;
        Object o1 = POP_OBJ ();
        VALIDATE (&o1, imm_int);
        Object ret = {.attr = {.type = imm_int, .gc = 0}, .value = NULL};

        ret.value = (void *)fn (&o1);
        PUSH_OBJ (ret);
        break;
      }
    case prim_gpio_config:
      {
        break;
      }
    case prim_gpio_set:
      {
        func_3_args_with_ret_t fn = (func_3_args_with_ret_t)prim->fn;
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        VALIDATE (&o2, imm_int);
        VALIDATE (&o1, symbol);
        Object ret = {.attr = {.type = imm_int, .gc = 0}, .value = NULL};
        ret = *(fn (&ret, &o1, &o2));
        PUSH_OBJ (ret);
        break;
      }
    case prim_gpio_toggle:
      {
        func_2_args_with_ret_t fn = (func_2_args_with_ret_t)prim->fn;
        Object o1 = POP_OBJ ();
        VALIDATE (&o1, symbol);
        Object ret = {.attr = {.type = imm_int, .gc = 0}, .value = NULL};
        ret = *(fn (&ret, &o1));
        PUSH_OBJ (ret);
        break;
      }
    case prim_get_board_id:
      {
        func_0_args_with_ret_t fn = (func_0_args_with_ret_t)prim->fn;
        Object ret = {.attr = {.type = mut_string, .gc = 0}, .value = NULL};
        ret = *(fn ());
        if (ret.attr.type != mut_string)
          {
            os_printk ("primitive: get-board-id type wrong\n");
          }
        PUSH_OBJ (ret);
        break;
      }

    default:
      os_printk ("Invalid prim number: %d\n", pn);
    }
}

static uintptr_t vm_get_uintptr (vm_t vm)
{
  u8_t buf[sizeof (uintptr_t)] = {0};

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

static u16_t vm_get_u16 (vm_t vm)
{
  u8_t buf[sizeof (u16_t)] = {0};

#if defined LAMBDACHIP_BIG_ENDIAN
  buf[0] = NEXT_DATA ();
  buf[1] = NEXT_DATA ();
#else
  buf[1] = NEXT_DATA ();
  buf[0] = NEXT_DATA ();
#endif
  return *((u16_t *)buf);
}

static object_t generate_object (vm_t vm, object_t obj)
{
  bytecode8_t bc;
  bc.all = NEXT_DATA ();
  obj->attr.gc = (VM_INIT_GLOBALS == vm->state) ? 3 : 0;
  obj->attr.type = bc.all;

  switch (obj->attr.type)
    {
    case imm_int:
      {
        imm_int_t value = (imm_int_t)vm_get_uintptr (vm);
        VM_DEBUG ("(push-integer-object %d)\n", value);
        obj->value = (void *)value;
        break;
      }
    case string:
      {
        const char *str = (char *)(vm->code + vm->pc);
        vm->pc += os_strnlen (str, MAX_STR_LEN) + 1;
        VM_DEBUG ("(push-string-object \"%s\")\n", str);
        obj->value = (void *)str;
        break;
      }
    case procedure:
      {
        u16_t offset = vm_get_u16 (vm);
        u8_t arity = NEXT_DATA ();
        u8_t opt = NEXT_DATA ();
        VM_DEBUG ("(push-proc-object 0x%x %d %d)\n", offset, arity, opt);
        obj->proc.entry = offset;
        obj->proc.arity = arity;
        obj->proc.opt = opt;
        break;
      }
    case primitive:
      {
        uintptr_t prim = vm_get_uintptr (vm);
        // vm->pc += sizeof (uintptr_t);
        VM_DEBUG ("(push-prim-object %u %s)\n", prim, prim_name (prim));
        obj->value = (void *)prim;
        break;
      }
    case list:
      {
        u8_t s = NEXT_DATA ();
        u16_t size = ((s << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-list-object %d)\n", size);
        list_t l = NEW (list);
        SLIST_INIT (&l->list);
        obj->attr.type = list;
        obj->value = (void *)l;

        for (u16_t i = 0; i < size; i++)
          {
            // object_t new_obj = NEW_OBJ (0);
            object_t new_obj = (object_t)GC_MALLOC (sizeof (Object));
            *new_obj = POP_OBJ ();
            new_obj->attr.gc = 1; // don't forget
            obj_list_t bl = (obj_list_t)GC_MALLOC (sizeof (ObjectList));
            bl->obj = new_obj;
            SLIST_INSERT_HEAD (&l->list, bl, next);
          }
        break;
      }
    case vector:
      {
        u8_t s = NEXT_DATA ();
        u16_t size = ((s << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-vector-object %d)\n", size);
        vector_t v = NEW (vector);
        v->vec = (object_t *)GC_MALLOC (sizeof (Object) * size);
        v->size = size;
        obj->attr.type = vector;
        obj->value = (void *)v;

        for (u16_t i = 0; i < size; i++)
          {
            object_t new_obj = (object_t)GC_MALLOC (sizeof (Object));
            *new_obj = POP_OBJ ();
            new_obj->attr.gc = 1; // don't forget
            v->vec[i] = new_obj;
          }
        break;
      }
    case real:
      {
        real_t r = {.v = vm_get_uintptr (vm)};
        VM_DEBUG ("(push-real-object %f)\n", r.f);
        obj->value = (void *)r.v;
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        numerator_t n = (u16_t)vm_get_u16 (vm);
        denominator_t d = (u16_t)vm_get_u16 (vm);
        hov_t value = ((n << 16) | d);
        VM_DEBUG ("(push-rational-object %d/%d)\n", n > 0 ? n : -n, d);
        obj->value = (void *)value;
        break;
      }
    case complex_exact:
      {
        real_part_t r = (real_part_t)vm_get_u16 (vm);
        imag_part_t i = (imag_part_t)vm_get_u16 (vm);
        hov_t value = ((r << 0xf) | i);
        VM_DEBUG ("(push-complex-object %d%d)\n", r, i);
        obj->value = (void *)value;
        break;
      }
    case complex_inexact:
      {
        const void *value = (void *)(vm->code + vm->pc);
#ifdef LAMBDACHIP_DEBUG
        real_t r = {.v = vm_get_uintptr (vm)};
        real_t i = {.v = vm_get_uintptr (vm)};
        if (i.f >= 0)
          {
            VM_DEBUG ("(push-complex-object %.1f+%.1fi)\n", r.f, i.f);
          }
        else
          {
            VM_DEBUG ("(push-complex-object %.1f%.1fi)\n", r.f, i.f);
          }
#else
        vm->pc += 8;
#endif
        obj->value = (void *)value;
        break;
      }
    default:
      {
        os_printk ("Oops, invalid object %d %d!\n", bc.type, bc.data);
        VM_PANIC ();
      }
    }

  return obj;
}

static void interp_single_encode (vm_t vm, bytecode8_t bc)
{
  switch (bc.type)
    {
    case LOCAL_REF:
      {
        VM_DEBUG ("(local %d)\n", bc.data);
        object_t obj = (object_t)LOCAL (bc.data);
        /* os_printk ("\nobj: "); */
        /* object_printer (obj); */
        /* os_printk ("\n"); */
        /* if (vm->closure) */
        /*   { */
        /*     for (int i = 0; i < vm->closure->frame_size; i++) */
        /*       { */
        /*         os_printk ("env[%d] type: %d, value: %d\n", i, */
        /*                 vm->closure->env[i].attr.type, */
        /*                 (imm_int_t) (vm->closure->env[i].value)); */
        /*       } */
        /*   } */
        PUSH_OBJ (*obj);
        break;
      }
    case LOCAL_REF_EXTEND:
      {
        VM_DEBUG ("(local %d)\n", bc.data);
        object_t obj = (object_t)LOCAL (bc.data + 16);
        PUSH_OBJ (*obj);
        break;
      }
    case CALL_LOCAL:
      {
        VM_DEBUG ("(call-local %d)\n", bc.data);
        object_t obj = (object_t)LOCAL (bc.data);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    case CALL_LOCAL_EXTEND:
      {
        VM_DEBUG ("(call-local %d)\n", bc.data + 16);
        object_t obj = (object_t)LOCAL (bc.data + 16);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    case FREE_REF:
      {
        u8_t frame = NEXT_DATA ();
        u8_t up = (frame & 0b00111111);
        u8_t offset = ((bc.data << 2) | ((frame & 0b11000000) >> 6));
        VM_DEBUG ("(free %x %d)\n", up, offset);
        object_t obj = (object_t)FREE_VAR (up, offset);
        /* os_printk ("obj: type = %d, value = %d\n", obj->attr.type, */
        /*         (imm_int_t)obj->value); */
        PUSH_OBJ (*obj);
        break;
      }
    case CALL_FREE:
      {
        /* TODO:
         *   1. For proc, what's stored in free?
         *   2. What's call convention?
         *   3. Do we need to create proc-object when store?
         */
        u8_t frame = NEXT_DATA ();
        u8_t up = (frame & 0b00111111);
        u8_t offset = ((bc.data << 2) | ((frame & 0b11000000) >> 6));
        VM_DEBUG ("(call-free %x %d)\n", up, offset);
        object_t obj = (object_t)FREE_VAR (up, offset);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    case LOCAL_ASSIGN:
      {
        u8_t offset_0 = NEXT_DATA ();
        u8_t offset = ((bc.data << 8) | offset_0);
        VM_DEBUG ("(assign-local %x)\n", offset);
        object_t obj = (object_t)LOCAL (offset);
        *obj = POP_OBJ ();
        break;
      }
    case FREE_ASSIGN:
      {
        u8_t frame = NEXT_DATA ();
        u8_t up = (frame & 0b00111111);
        u8_t offset = ((bc.data << 2) | ((frame & 0b11000000) >> 6));
        VM_DEBUG ("(assign-free %x %d)\n", up, offset);
        object_t obj = (object_t)FREE_VAR (up, offset);
        *obj = POP_OBJ ();
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X\n", bc.all);
        panic ("interp_single_encode panic!\n");
      }
    }
}

static void interp_double_encode (vm_t vm, bytecode16_t bc)
{
  switch (bc.type)
    {
    case PRELUDE:
      {
        VM_DEBUG ("(prelude %d %d)\n", PROC_MODE (bc.bc2), PROC_ARITY (bc.bc2));
        SAVE_ENV ();
        break;
      }
    case LOCAL_REF_HIGH:
      {
        u8_t offset = bc.bc2 + 32;
        VM_DEBUG ("(local %d)\n", offset);
        object_t obj = (object_t)LOCAL (offset);
        PUSH_OBJ (*obj);
        break;
      }
    case CALL_LOCAL_HIGH:
      {
        u8_t offset = bc.bc2 + 32;
        VM_DEBUG ("(call-local %d)\n", offset);
        object_t obj = (object_t)LOCAL (offset);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    case GLOBAL_VAR_REF:
      {
        u8_t index = bc.bc2;
        VM_DEBUG ("(global %d)\n", index);
        object_t obj = &GLOBAL (index);
        PUSH_OBJ (*obj);
        break;
      }
    case GLOBAL_VAR_ASSIGN:
      {
        u8_t index = bc.bc2;
        Object var = POP_OBJ ();
#ifdef LAMBDACHIP_DEBUG
        os_printk ("(global-assign %d ", index);
        object_printer (&var);
        os_printk (")\n");
#endif
        GLOBAL_ASSIGN (index, var);
        PUSH_OBJ (GLOBAL_REF (none_const)); // return NONE object
        break;
      }
    case CALL_GLOBAL_VAR:
      {
        u8_t index = bc.bc2;
        VM_DEBUG ("(call-global %d)\n", index);
        object_t obj = &GLOBAL (index);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X %X\n", bc.type, bc.data);
        panic ("interp_double_encode panic!\n");
      }
    };
}

static void interp_triple_encode (vm_t vm, bytecode24_t bc)
{
  switch (bc.type)
    {
    case VEC_REF:
      {
        panic ("VEC_REf hasn't been implemented yet!");
        // VM_DEBUG ("(vec-ref 0x%p %d)\n", vec, bc.bc3);
        // PUSH (vector_ref (vec, bc.bc2));
        break;
      }
    case CALL_PROC:
      {
        u32_t offset = bc.data;
        VM_DEBUG ("(call-proc 0x%x)\n", offset);
        /* os_printk ("call-proc before fp: %d\n", ((u32_t
         * *)vm->stack)[vm->fp]);
         */
        FIX_PC ();
        PROC_CALL (offset);
        /* os_printk ("call-proc after fp: %d\n", ((u32_t *)vm->stack)[vm->fp]);
         */
        break;
      }
    case F_JMP:
      {
        u32_t offset = bc.data;
        VM_DEBUG ("(fjump 0x%x)\n", offset);
        Object obj = POP_OBJ ();

        if (is_false (&obj))
          {
            VM_DEBUG ("False! Jump!\n");
            JUMP (offset);
          }

        break;
      }
    case JMP:
      {
        u32_t offset = bc.data;
        VM_DEBUG ("(jump 0x%x)\n", offset);
        JUMP (offset);
        break;
      }
    case GLOBAL_VAR_REF_EXTEND:
      {
        u32_t index = NEXT_DATA () + 256;
        VM_DEBUG ("(global-assign %d)\n", index);
        object_t obj = &GLOBAL (index);
        PUSH_OBJ (*obj);
        break;
      }
    case GLOBAL_VAR_ASSIGN_EXTEND:
      {
        u32_t index = NEXT_DATA () + 256;
        Object var = POP_OBJ ();
#ifdef LAMBDACHIP_DEBUG
        os_printk ("(global-assign %d ", index);
        object_printer (&var);
        os_printk (")\n");
#endif
        GLOBAL_ASSIGN (index, var);
        PUSH_OBJ (GLOBAL_REF (none_const)); // return NONE object
        break;
      }
    case CALL_GLOBAL_VAR_EXTEND:
      {
        u32_t index = NEXT_DATA () + 256;
        VM_DEBUG ("(call-global %d)\n", index);
        object_t obj = &GLOBAL (index);
        if (NEED_VARGS (obj))
          handle_optional_args (vm, obj);
        CALL (obj);
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X, %X, %X\n", bc.bc1, bc.bc2, bc.bc3);
        panic ("interp_triple_encode panic!\n");
      }
    }
}

static void interp_quadruple_encode (vm_t vm, bytecode32_t bc)
{
  switch (bc.type)
    {
    case VEC_SET:
      {
        panic ("VEC_SET hasn't been implemented yet!");
        /* vector_t vec = (vector_t)ss_read_u32 (bc.bc1); */
        /* object_t obj = (object_t)ss_read_u32 (bc.bc3); */
        /* VM_DEBUG ("(vec-set! 0x%p %d 0x%p)\n", vec, bc.bc2, obj); */
        // vector_set (vec, bc.bc2, obj);
        break;
      }
    case CLOSURE_ON_STACK:
      {
        u8_t size = (bc.bc2 & 0xFF);
        u8_t arity = ((bc.bc2 & 0xFF00) >> 4);
        reg_t entry = ((bc.bc3 << 8) | bc.bc4);
        reg_t env = vm->sp + sizeof (Object); // skip closure object
        VM_DEBUG ("(closure-on-stack %d 0x%x)\n", size, entry);
        Object obj = {.attr = {.type = closure_on_stack, .gc = 0},
                      .value = (void *)((env | (size << 10) | (entry << 16)))};
        PUSH_OBJ (obj);
        break;
      }
    case CLOSURE_ON_HEAP:
      {
        u8_t size = (bc.bc2 & 0xF);
        u8_t arity = ((bc.bc2 & 0xF0) >> 4);
        reg_t entry = ((bc.bc3 << 8) | bc.bc4);
        VM_DEBUG ("(closure-on-heap %d %d 0x%x)\n", arity, size, entry);
        closure_t closure = create_closure (vm, arity, size, entry);
        Object obj = {.attr = {.type = closure_on_heap, .gc = 0},
                      .value = (closure_t)closure};
        PUSH_OBJ (obj);
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X, %X, %X, %X\n", bc.bc1, bc.bc2, bc.bc3,
                   bc.bc4);
        panic ("interp_quadruple_encode panic!\n");
      }
    }
}

static void interp_special (vm_t vm, bytecode8_t bc)
{
  switch (bc.type)
    {
    case PRIMITIVE:
      {
        VM_DEBUG ("(primitive %d %s)\n", bc.data, prim_name (bc.data));
        call_prim (vm, (pn_t)bc.data);
        /* os_printk ("result: "); */
        /* object_printer (TOP_OBJ_PTR ()); */
        /* os_printk ("\n"); */

        break;
      }
    case PRIMITIVE_EXT:
      {
        u8_t pn_low = NEXT_DATA ();
        u16_t pn = ((bc.data & 0xF) << 8 | pn_low) + 16;
        VM_DEBUG ("(primitive-ext %d %s)\n", pn, prim_name (pn));
        call_prim (vm, pn);
        /* os_printk ("result: "); */
        /* object_printer (TOP_OBJ_PTR ()); */
        /* os_printk ("\n"); */
        break;
      }
    case OBJECT:
      {
        switch (bc.data)
          {
          case GENERAL_OBJECT:
            {
              Object obj = {0};
              generate_object (vm, &obj);
              PUSH_OBJ (obj);
              break;
            }
          case FALSE:
            {
              VM_DEBUG ("(push-boolean-false)\n");
              Object obj = GLOBAL_REF (false_const);
              PUSH_OBJ (obj);
              break;
            }
          case TRUE:
            {
              VM_DEBUG ("(push-boolean-true)\n");
              Object obj = GLOBAL_REF (true_const);
              PUSH_OBJ (obj);
              break;
            }
          case SYMBOL:
            {
              Object sym = {.attr = {.type = symbol, .gc = 0}, .value = NULL};
              u16_t offset = vm_get_u16 (vm);
              const char *str_buf = GET_SYMBOL (offset);
              VM_DEBUG ("(push-symbol-object #{%s}#)\n", str_buf);
              make_symbol (str_buf, &sym);
              sym.value = (void *)offset;
              PUSH_OBJ (sym);
              break;
            }
          }
        break;
      }
    case CONTROL:
      {
        switch (bc.data)
          {
          case HALT:
            {
              if (VM_INIT_GLOBALS != vm->state)
                {
                  VM_DEBUG ("GC clean!\n");
                  gc_clean_cache ();
                  VM_DEBUG ("Halt here!\n");
                }
              vm->state = VM_STOP;
              break;
            }
          default:
            break;
          }
        break;
      }
    default:
      {
        os_printk ("Invalid special bytecode %X, %X\n", bc.type, bc.data);
        panic ("interp_special_encode panic!\n");
      }
    }
}

static bytecode8_t fetch_next_bytecode (vm_t vm)
{
  static bytecode8_t bc = {0};

  // os_printk ("pc: %d\n", vm->pc);
  if (vm->pc < GLOBAL_REF (VM_CODESEG_SIZE))
    {
      bc.all = vm->code[vm->pc++];
      // os_printk ("BC type(%x) data(%x)\n", bc.type, bc.data);
    }
  else
    {
      os_printk ("Oops, no more bytecode!\n");
      VM_PANIC ();
    }

  return bc;
}

void vm_init_environment (vm_t vm)
{
  vm->fetch_next_bytecode = fetch_next_bytecode;
  vm->state = VM_RUN;
  vm->sp = 0;
  vm->fp = 0;
  vm->local = 0;
  vm->shadow = 0;
  vm->cc = NULL;
  vm->closure = NULL;
  vm->tail_rec = false;
}

void vm_init (vm_t vm)
{
  os_memset (vm, 0, sizeof (struct LambdaVM));
  vm_init_environment (vm);
  vm->code = (u8_t *)os_malloc (GLOBAL_REF (VM_CODESEG_SIZE));
  vm->data = (u8_t *)os_malloc (GLOBAL_REF (VM_DATASEG_SIZE));
  vm->stack = (u8_t *)os_malloc (GLOBAL_REF (VM_STKSEG_SIZE));
  vm->globals = NULL;

  SLIST_INIT (&closure_stack);
}

void vm_clean (vm_t vm)
{
  os_free (vm->code);
  vm->code = NULL;

  os_free (vm->data);
  vm->data = NULL;

  os_free (vm->stack);
  vm->stack = NULL;

  os_free (vm->globals);
  vm->globals = NULL;
}

void vm_init_globals (vm_t vm, lef_t lef)
{
  u8_t *stack = vm->stack; // backup stack
  os_memcpy (vm->code, LEF_GLOBAL (lef), lef->gsize);
  vm->pc = 0;
  vm->state = VM_INIT_GLOBALS;

  vm_run (vm);

  /* #ifdef LAMBDACHIP_DEBUG */
  /*   os_printk ("Globals %d: sp: %d\n", vm->sp / sizeof (Object), vm->sp); */
  /*   for (u32_t i = 0; i < vm->sp / sizeof (Object); i++) */
  /*     { */
  /*       object_printer (&GLOBAL (i)); */
  /*       os_printk ("\n"); */
  /*     } */
  /* #endif */

  size_t size = vm->sp;
  vm->globals = (object_t)os_malloc (size);
  os_memcpy (vm->globals, vm->stack, size);

  vm_init_environment (vm);
}

void vm_load_lef (vm_t vm, lef_t lef)
{
  create_symbol_table (&lef->symtab);
  GLOBAL_REF (symtab) = &lef->symtab;
  // FIXME: not all mem section is data seg
  os_memcpy (vm->data, LEF_MEM (lef), lef->msize);

  vm_init_globals (vm, lef);

  // FIXME: Check size boundary
  os_memcpy (vm->code, LEF_PROG (lef), lef->psize);
  vm->pc = lef->entry;
}

void vm_restart (vm_t vm)
{
  /* TODO:
   * 1. Free all objects in the heap
   * 2. Clean all global information, include ss
   */
  vm_init_environment (vm);
}

static encode_t pre_fetch (vm_t vm, bytecode8_t bytecode)
{
  //  VM_DEBUG("Prefetch: %x\n", bytecode.all);
  if (SINGLE_ENCODE (bytecode))
    {
      return SINGLE;
    }
  if (DOUBLE_ENCODE (bytecode))
    {
      return DOUBLE;
    }
  else if (TRIPLE_ENCODE (bytecode))
    {
      return TRIPLE;
    }
  else if (QUADRUPLE_ENCODE (bytecode))
    {
      return QUADRUPLE;
    }
  else if (IS_SPECIAL (bytecode))
    {
      return SPECIAL;
    }

  VM_DEBUG ("Invalid encode %x\n", bytecode.type);
  panic ("BUG in pre_fetch: it's impossible to be here!");
  return -1;
}

static void dispatch (vm_t vm, bytecode8_t bc)
{
  switch (pre_fetch (vm, bc))
    {
    case SINGLE:
      {
        interp_single_encode (vm, bc);
        break;
      }
    case DOUBLE:
      {
        bytecode16_t bc16;
        bc16.bc1 = bc.all;
        bc16.bc2 = NEXT_DATA ();
        interp_double_encode (vm, bc16);
        break;
      }
    case TRIPLE:
      {
        bytecode24_t bc24;
        bc24.bc1 = bc.all;
        bc24.bc2 = NEXT_DATA ();
        bc24.bc3 = NEXT_DATA ();
        interp_triple_encode (vm, bc24);
        break;
      }
    case QUADRUPLE:
      {
        bytecode32_t bc32;
        bc32.bc1 = bc.all;
        bc32.bc2 = NEXT_DATA ();
        bc32.bc3 = NEXT_DATA ();
        bc32.bc4 = NEXT_DATA ();
        interp_quadruple_encode (vm, bc32);
        break;
      }
    case SPECIAL:
      interp_special (vm, bc);
      break;
    default:
      {
        os_printk ("Invalid bytecode type!\n");
        panic ("vm_run panic!\n");
      }
    };
}

void vm_load_compiled_file (const char *filename)
{
  os_printk ("Lambdachip hasn't supported file loading yet!\n");
}

void vm_run (vm_t vm)
{
  VM_DEBUG ("VM run!\n");

  while (VM_RUN == vm->state || VM_INIT_GLOBALS == vm->state)
    {
      /* TODO:
       * 1. Add debug info
       */
      dispatch (vm, FETCH_NEXT_BYTECODE ());
      /* os_printk ("pc: %d, local: %d, sp: %d, fp: %d\n", vm->pc, vm->local, */
      /*            vm->sp, vm->fp); */
      /* os_printk ("----------LOCAL------------\n"); */
      /* u32_t bound = (vm->sp - (vm->fp ? vm->fp + FPS : 0)); */
      /* for (u32_t i = 0; i < bound / 8; i++) */
      /*   { */
      /*     object_t obj = (object_t)LOCAL_FIX (i); */
      /*     os_printk ("obj: local = %d, type = %d, value = %d\n", */
      /*                vm->local + i * 8, obj->attr.type,
       * (imm_int_t)obj->value); */
      /*   } */
      /* os_printk ("------------END-----------\n"); */
      /* getchar (); */

      if (!vm->sp)
        {
          VM_DEBUG ("stack is empty, try to recycle once!\n");
          gc_try_to_recycle ();
          VM_DEBUG ("done\n");
        }
    }
}

void apply_proc (vm_t vm, object_t proc, object_t ret)
{
  // TODO: run proc with a new stack, and the code snippet of
  u16_t entry = proc->proc.entry;

  vm->pc = proc->proc.entry;

  while (VM_RUN == vm->state)
    {
      bytecode8_t bc = FETCH_NEXT_BYTECODE ();

      if (IS_PROC_END (bc))
        break;

      dispatch (vm, bc);
      /* os_printk ("pc: %d, local: %d, sp: %d, fp: %d\n", vm->pc, vm->local,
       * vm->sp, */
      /*         vm->fp); */
      /* os_printk ("----------LOCAL------------\n"); */
      /* u32_t bound = (vm->sp - (vm->fp ? vm->fp + FPS : 0)); */
      /* for (u32_t i = 0; i < bound / 8; i++) */
      /*   { */
      /*     object_t obj = (object_t)LOCAL_FIX (i); */
      /*     os_printk ("obj: local = %d, type = %d, value = %d\n", vm->local +
       * i * 8, */
      /*             obj->attr.type, (imm_int_t)obj->value); */
      /*   } */
      /* os_printk ("------------END-----------\n"); */
      /* getchar (); */
    }

  if (ret)
    {
      *ret = POP_OBJ ();
      /* NOTE: Since we use the deref tick here for copying,
       *       we must reset gc to 1 again!!! */
      ret->attr.gc = 1;
    }
  else
    {
      POP_OBJ ();
    }
}
