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

GLOBAL_DEF (size_t, VM_CODESEG_SIZE) = 0;
GLOBAL_DEF (size_t, VM_DATASEG_SIZE) = 0;
GLOBAL_DEF (size_t, VM_GLOBALSEG_SIZE) = 0;

static void handle_optional_args (vm_t vm, object_t proc)
{
  u8_t cnt = COUNT_ARGS () - proc->proc.opt;
  Object varg = {.attr = {.type = list, .gc = FREE_OBJ},
                 .value = (void *)NEW_INNER_OBJ (list)};
  ListHead *head = LIST_OBJECT_HEAD (&varg);

  for (int i = 0; i < cnt; i++)
    {
      object_t new_obj = NEW_OBJ (0);
      *new_obj = POP_OBJ ();
      new_obj->attr.gc = GEN_1_OBJ; // don't forget to reset gc to GEN_1_OBJ
      list_node_t bl = (list_node_t)GC_MALLOC (sizeof (ListNode));
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

void call_prim (vm_t vm, pn_t pn)
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
    case int_add:
    case int_sub:
    case int_mul:
    case fract_div:
      {
        func_2_args_with_ret_t fn = (func_2_args_with_ret_t)prim->fn;
        size_t size = sizeof (struct Object);
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        Object ret = {.attr = {.type = none, .gc = FREE_OBJ}, .value = NULL};
        ret = *(fn (vm, &ret, &o1, &o2));
        PUSH_OBJ (ret);
        break;
      }
    case int_modulo:
    case int_remainder:
      {
        arith_prim_t fn = (arith_prim_t)prim->fn;
        size_t size = sizeof (struct Object);
        Object xsor = POP_OBJ ();
        Object xend = POP_OBJ ();
        Object ret = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = NULL};
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
        // FIXME: Fix the stack order for GC

        Object lst = POP_OBJ ();
        Object proc = POP_OBJ ();
        ListHead *head = LIST_OBJECT_HEAD (&lst);
        list_node_t node = NULL;
        list_node_t prev = NULL;
        /* We always set k as return */
        Object k = GEN_PRIM (ret);
        list_t new_list = NEW_INNER_OBJ (list);
        /* NOTE:
         * To safely created a List, we have to consider that GC may happend
         * unexpectedly.
         */
        Object new_list_obj = {.attr = {.type = list, .gc = PERMANENT_OBJ},
                               .value = (void *)new_list};
        ListHead *new_head = LIST_OBJECT_HEAD (&new_list_obj);
        new_list->non_shared = 0;

        SAVE_ENV_SIMPLE ();

        SLIST_FOREACH (node, head, next)
        {
          list_node_t new_node = NEW_LIST_NODE ();
          new_node->obj = (void *)0xDEADBEEF;
          if (!prev)
            {
              // when the new list is still empty
              SLIST_INSERT_HEAD (new_head, new_node, next);
            }
          else
            {
              SLIST_INSERT_AFTER (prev, new_node, next);
            }

          object_t new_obj = NEW_OBJ (0);
          new_obj->attr.gc = GEN_1_OBJ;
          new_node->obj = new_obj;
          vm->sp = vm->local;

          PUSH_OBJ (k);
          PUSH_OBJ (*node->obj);

          switch (proc.attr.type)
            {
            case procedure:
              {
                apply_proc (vm, &proc, new_obj);
                break;
              }
            case primitive:
              {
                call_prim (vm, (pn_t)proc.value);
                *new_obj = POP_OBJ ();
                break;
              }
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

        new_list->attr.gc
          = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        PUSH_OBJ (new_list_obj);
        RESTORE_SIMPLE ();
        break;
      }
    case foreach:
      {
        /* We always set k as return */
        Object k = GEN_PRIM (ret);
        Object lst = POP_OBJ ();
        Object proc = POP_OBJ ();
        ListHead *head = LIST_OBJECT_HEAD (&lst);
        list_node_t node = NULL;

        PUSH_REG (vm->pc);
        PUSH_REG (vm->fp);
        PUSH (vm->attr.all);
        PUSH_CLOSURE (vm->closure);
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
        Object ret = CREATE_RET_OBJ ();
        ListHead *head = LIST_OBJECT_HEAD (&args);
        list_node_t node = NULL;

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
              PANIC ("apply panic!\n");
            }
          }
        break;
      }
    case prim_bytevector_copy_overwrite: // 5 parameters
      {
        func_5_args_with_ret_t fn = (func_5_args_with_ret_t)prim->fn;
        Object o5 = POP_OBJ ();
        Object o4 = POP_OBJ ();
        Object o3 = POP_OBJ ();
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        Object ret = CREATE_RET_OBJ ();
        ret = *(fn (vm, &ret, &o1, &o2, &o3, &o4, &o5));
        PUSH_OBJ (ret);
        break;
      }
    case prim_i2c_write_byte: // 4 parameters
    case prim_spi_transceive:
      {
        func_4_args_with_ret_t fn = (func_4_args_with_ret_t)prim->fn;
        Object o4 = POP_OBJ ();
        Object o3 = POP_OBJ ();
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        Object ret = CREATE_RET_OBJ ();
        ret = *(fn (vm, &ret, &o1, &o2, &o3, &o4));
        PUSH_OBJ (ret);
        break;
      }
    case prim_i2c_read_list: // 3 parameters
    case prim_i2c_read_bytevector:
    case prim_i2c_write_list:
    case prim_i2c_read_byte:
    case prim_bytevector_u8_set:
    case prim_bytevector_copy:
    case prim_i2c_write_bytevector:
    case prim_string_set:
    case prim_substring:
      {
        func_3_args_with_ret_t fn = (func_3_args_with_ret_t)prim->fn;
        Object o3 = POP_OBJ ();
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        Object ret = CREATE_RET_OBJ ();
        ret = *(fn (vm, &ret, &o1, &o2, &o3));
        PUSH_OBJ (ret);
        break;
      }
    case list_append: // 2 parameters
    case list_ref:
    case cons:
    case prim_gpio_set:
    case prim_make_bytevector:
    case prim_bytevector_u8_ref:
    case prim_bytevector_append:
    case prim_floor_div:
    case prim_floor_quotient:
    case prim_floor_remainder:
    case prim_truncate_div:
    case prim_truncate_quotient:
    case prim_truncate_remainder:
    case prim_expt:
    case prim_make_string:
    case prim_string_ref:
    case prim_string_eq:
      {
        func_2_args_with_ret_t fn = (func_2_args_with_ret_t)prim->fn;
        Object o2 = POP_OBJ ();
        Object o1 = POP_OBJ ();
        Object ret = CREATE_RET_OBJ ();
        fn (vm, &ret, &o1, &o2);
        PUSH_OBJ (ret);
        break;
      }
    case prim_usleep: // 1 parameter
    case prim_device_configure:
    case prim_gpio_toggle:
    case prim_gpio_get:
    case list_to_string:
    case car:
    case cdr:
    case prim_bytevector_length:
    case prim_floor:
    case prim_ceiling:
    case prim_truncate:
    case prim_round:
    case prim_rationalize:
    case prim_numerator:
    case prim_denominator:
    case prim_is_exact_integer:
    case prim_is_finite:
    case prim_is_infinite:
    case prim_is_nan:
    case prim_is_zero:
    case prim_is_positive:
    case prim_is_negative:
    case prim_is_odd:
    case prim_is_even:
    case prim_square:
    case prim_sqrt:
    case prim_exact_integer_sqrt:
    case prim_string_length:
      {
        func_1_args_with_ret_t fn = (func_1_args_with_ret_t)prim->fn;
        Object o = POP_OBJ ();
        Object ret = CREATE_RET_OBJ ();
        PUSH_OBJ (*fn (vm, &ret, &o));
        break;
      }
    case prim_get_board_id:
      {
        func_0_args_t fn = (func_0_args_t)prim->fn;
        PUSH_OBJ (*fn (vm));
        break;
      }
    case read_char:
    case readln:
    case prim_vm_reset:
      {
        func_0_args_with_ret_t fn = (func_0_args_with_ret_t)prim->fn;
        Object ret = CREATE_RET_OBJ ();
        PUSH_OBJ (*fn (vm, &ret));
        break;
      }
    case scm_raise:
      {
        vm->state = VM_EXCPT;
        break;
      }
    case scm_raise_continuable:
      {
        vm->state = VM_EXCPT_CONT;
        break;
      }
    case with_exception_handler:
      {
        Object thunk = POP_OBJ ();
        Object handler = POP_OBJ ();
        Object result = CREATE_RET_OBJ ();
        Object k = GEN_PRIM (restore);
        reg_t local = 0;

        SAVE_ENV_SIMPLE ();
        local = vm->local;
        PUSH_OBJ (k); // prim:return
        apply_proc (vm, &thunk, &result);

      extent:
        if (VM_EXCPT_CONT == vm->state)
          {
            reg_t sp = POP_REG ();
            reg_t pc = POP_REG ();
            SAVE_ENV_SIMPLE ();
            PUSH_OBJ (k); // prim:return
            PUSH_OBJ (result);
            vm->state = VM_RUN;
            apply_proc (vm, &handler, &result);
            PUSH_OBJ (result);
            RESTORE_SIMPLE ();
            vm->sp = sp;
            vm->local = local;
            PUSH_OBJ (result);
            thunk.proc.entry = pc;
            apply_proc (vm, &thunk, &result);
            goto extent;
          }
        else if (VM_EXCPT == vm->state)
          {
            SAVE_ENV_SIMPLE ();
            PUSH_OBJ (k); // prim:return
            PUSH_OBJ (result);
            vm->state = VM_RUN;
            apply_proc (vm, &handler, NULL);
            RESTORE_SIMPLE ();
            // Handle non-continuable exception
            os_printk ("Exception occurred with non-continuable object: ");
            object_printer (&result);
            os_printk ("\n");
            vm->state = VM_STOP;
          }

        PUSH_OBJ (result);
        RESTORE_SIMPLE ();
        break;
      }
    case is_null:
    case is_pair:
    case is_list:
    case is_string:
    case is_char:
    case is_keyword:
    case is_symbol:
    case is_procedure:
    case is_primitive:
    case is_boolean:
    case is_number:
    case is_integer:
    case is_real:
    case is_rational:
    case is_complex:
    case is_bytevector:
      {
        Object o = POP_OBJ ();
        pred_t fn = (pred_t)prim->fn;
        PUSH_OBJ (fn (&o));
        break;
      }
    default:
      {
        PANIC ("Invalid prim number: %d\n", pn);
      }
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
  obj->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : FREE_OBJ;
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
    case string: // immutable string
      {
        const char *str = (char *)(vm->code + vm->pc);
        vm->pc += os_strnlen (str, MAX_STR_LEN) + 1;
        VM_DEBUG ("(push-string-object \"%s\")\n", str);
        obj->value = (void *)str;
        break;
      }
    case keyword:
      {
        const char *str = (char *)(vm->code + vm->pc);
        vm->pc += os_strnlen (str, MAX_STR_LEN) + 1;
        VM_DEBUG ("(push-keyword-object #:%s)\n", str);
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
    case pair:
      {
        VM_DEBUG ("(push-pair-object)\n");
        pair_t p = NEW_INNER_OBJ (pair);
        p->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        obj->attr.type = pair;
        obj->value = (void *)p;
        PUSH_OBJ (*obj);
        u32_t sp = vm->sp - sizeof (Object);

        object_t cdr = NEW_OBJ (TOP_OBJ_PTR_FROM (sp)->attr.type);
        *cdr = POP_OBJ_FROM (sp);
        cdr->attr.gc = GEN_1_OBJ; // don't forget to reset gc to 1
        p->cdr = cdr;

        object_t car = NEW_OBJ (TOP_OBJ_PTR_FROM (sp)->attr.type);
        *car = POP_OBJ_FROM (sp);
        car->attr.gc = GEN_1_OBJ; // don't forget to reset gc to 1
        p->car = car;

        vm->sp = sp; // refix the pop offset
        break;
      }
    case list:
      {
        u8_t s = NEXT_DATA ();
        u16_t size = ((s << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-list-object %d)\n", size);
        list_t l = NEW_INNER_OBJ (list);
        SLIST_INIT (&l->list);
        l->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        l->non_shared = 0;
        obj->attr.type = list;
        obj->value = (void *)l;

        /* NOTE:
         * To safely created a List, we have to consider that GC may happend
         * unexpectedly.
         * 1. We must save list-obj to avoid to be freed by GC.
         * 2. The POP operation must be fixed to skip list-obj.
         * 3. oln must be created and inserted before the object allocation.
         */
        PUSH_OBJ (*obj);
        u32_t sp = vm->sp - sizeof (Object);

        for (u16_t i = 0; i < size; i++)
          {
            // POP_OBJ_FROM (sp);
            list_node_t bl = NEW_LIST_NODE ();
            // avoid crash in case GC was triggered here
            bl->obj = (void *)0xDEADBEEF;
            SLIST_INSERT_HEAD (&l->list, bl, next);
            object_t new_obj = NEW_OBJ (TOP_OBJ_PTR_FROM (sp)->attr.type);
            *new_obj = POP_OBJ_FROM (sp);
            // FIXME: What if it's global const?
            new_obj->attr.gc
              = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
            bl->obj = new_obj;
          }
        vm->sp = sp; // refix the pop offset
        break;
      }
    case vector:
      {
        u8_t s = NEXT_DATA ();
        u16_t size = ((s << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-vector-object %d)\n", size);
        vector_t v = NEW_INNER_OBJ (vector);
        v->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        v->vec = (object_t *)GC_MALLOC (sizeof (Object) * size);
        v->size = size;
        obj->attr.type = vector;
        obj->value = (void *)v;
        PUSH_OBJ (*obj);
        u32_t sp = vm->sp - sizeof (Object);

        for (u16_t i = 0; i < size; i++)
          {
            object_t new_obj = NEW_OBJ (TOP_OBJ_PTR_FROM (sp)->attr.type);
            *new_obj = POP_OBJ_FROM (sp);
            new_obj->attr.gc = GEN_1_OBJ; // don't forget to reset gc to 1
            v->vec[i] = new_obj;
          }
        vm->sp = sp; // refix the pop offset
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
    case mut_bytevector:
      {
        u16_t size = NEXT_DATA ();
        size = ((size << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-mut_bytevector-object %d)\n", size);
        mut_bytevector_t v = NEW_INNER_OBJ (mut_bytevector);
        v->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        v->size = size;
        obj->attr.gc = v->attr.gc;
        obj->attr.type = mut_bytevector;
        v->vec = (u8_t *)GC_MALLOC (size);
        obj->value = (void *)v;
        vm->pc += size;
        break;
      }
    case bytevector: // immutable bytevector
      {
        u16_t size = NEXT_DATA ();
        size = ((size << 8) | NEXT_DATA ());
        VM_DEBUG ("(push-bytevector-object %d)\n", size);
        bytevector_t v = NEW_INNER_OBJ (bytevector);
        v->attr.gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ : GEN_1_OBJ;
        v->size = size;
        obj->attr.gc = v->attr.gc;
        obj->attr.type = bytevector;
        v->vec = vm->code + vm->pc;
        obj->value = (void *)v;
        vm->pc += size;
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
    case LOCAL_ASSIGN:
      {
        u8_t offset_0 = NEXT_DATA ();
        u8_t offset = ((bc.data << 8) | offset_0);
        VM_DEBUG ("(assign-local %x)\n", offset);
        object_t obj = (object_t)LOCAL (offset);
        *obj = POP_OBJ ();
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X\n", bc.all);
        PANIC ("interp_single_encode panic!\n");
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
    case GLOBAL_VAR_ASSIGN:
      {
        u8_t index = bc.bc2;
        Object var = POP_OBJ ();
#ifdef LAMBDACHIP_DEBUG
        if (GLOBAL_REF (vm_verbose))
          {
            os_printk ("(global-assign %d ", index);
            object_printer (&var);
            os_printk (")\n");
          }
#endif
        GLOBAL_ASSIGN (index, var);
        PUSH_OBJ (GLOBAL_REF (none_const)); // return NONE object
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
        PANIC ("interp_double_encode panic!\n");
      }
    };
}

static void interp_triple_encode (vm_t vm, bytecode24_t bc)
{
  switch (bc.type)
    {
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
    case VEC_REF:
      {
        PANIC ("VEC_REf hasn't been implemented yet!");
        // VM_DEBUG ("(vec-ref 0x%p %d)\n", vec, bc.bc3);
        // PUSH (vector_ref (vec, bc.bc2));
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
    case GLOBAL_VAR_REF_EXTEND:
      {
        u32_t index = NEXT_DATA () + 256;
        VM_DEBUG ("(global-assign %d)\n", index);
        object_t obj = &GLOBAL (index);
        PUSH_OBJ (*obj);
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
        PANIC ("interp_triple_encode panic!\n");
      }
    }
}

static void interp_quadruple_encode (vm_t vm, bytecode32_t bc)
{
  switch (bc.type)
    {
    case VEC_SET:
      {
        PANIC ("VEC_SET hasn't been implemented yet!");
        /* vector_t vec = (vector_t)ss_read_u32 (bc.bc1); */
        /* object_t obj = (object_t)ss_read_u32 (bc.bc3); */
        /* VM_DEBUG ("(vec-set! 0x%p %d 0x%p)\n", vec, bc.bc2, obj); */
        // vector_set (vec, bc.bc2, obj);
        break;
      }
    case CLOSURE_ON_HEAP:
      {
        u8_t size = (bc.bc2 & 0xF);
        u8_t arity = ((bc.bc2 & 0xF0) >> 4);
        reg_t entry = ((bc.bc3 << 8) | bc.bc4);
        VM_DEBUG ("(closure-on-heap %d %d 0x%x)\n", arity, size, entry);
        closure_t closure = create_closure (vm, arity, size, entry);
        Object obj = {.attr = {.type = closure_on_heap, .gc = FREE_OBJ},
                      .value = (closure_t)closure};
        gc_inner_obj_book (closure_on_heap, closure);
        PUSH_OBJ (obj);
        break;
      }
    case CLOSURE_ON_STACK:
      {
        u8_t size = (bc.bc2 & 0xFF);
        u8_t arity = ((bc.bc2 & 0xFF00) >> 4);
        reg_t entry = ((bc.bc3 << 8) | bc.bc4);
        reg_t env = vm->sp + sizeof (Object); // skip closure object
        VM_DEBUG ("(closure-on-stack %d 0x%x)\n", size, entry);
        Object obj = {.attr = {.type = closure_on_stack, .gc = FREE_OBJ},
                      .value = (void *)((env | (size << 10) | (entry << 16)))};
        PUSH_OBJ (obj);
        break;
      }
    default:
      {
        os_printk ("Invalid bytecode %X, %X, %X, %X\n", bc.bc1, bc.bc2, bc.bc3,
                   bc.bc4);
        PANIC ("interp_quadruple_encode panic!\n");
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
              Object sym
                = {.attr = {.type = symbol,
                            .gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ
                                                                 : FREE_OBJ},
                   .value = NULL};
              u16_t offset = vm_get_u16 (vm);
              const char *str_buf = GET_SYMBOL (offset);
              VM_DEBUG ("(push-symbol-object %s)\n", str_buf);
              make_symbol (str_buf, &sym);
              PUSH_OBJ (sym);
              break;
            }
          case CHAR:
            {
              Object obj
                = {.attr = {.type = character,
                            .gc = (VM_INIT_GLOBALS == vm->state) ? PERMANENT_OBJ
                                                                 : FREE_OBJ},
                   .value = NULL};
              u8_t ch = NEXT_DATA ();
              obj.value = (void *)ch;
              PUSH_OBJ (obj);
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
        PANIC ("interp_special_encode panic!\n");
      }
    }
}

static bytecode8_t fetch_next_bytecode (vm_t vm)
{
  static bytecode8_t bc = {0};

  // os_printk ("pc: %d\n", vm->pc);
  if ((VM_RUN == vm->state && vm->pc < GLOBAL_REF (VM_CODESEG_SIZE))
      || (VM_INIT_GLOBALS == vm->state
          && vm->pc < GLOBAL_REF (VM_GLOBALSEG_SIZE)))
    {
      bc.all = vm->code[vm->pc++];
      // os_printk ("BC type(%x) data(%x)\n", bc.type, bc.data);
    }
  else
    {
      os_printk ("Oops, no more bytecode! pc: %d, global: %d, code: %d\n",
                 vm->pc, GLOBAL_REF (VM_GLOBALSEG_SIZE),
                 GLOBAL_REF (VM_CODESEG_SIZE));
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
  vm->attr.shadow = 0;
  vm->attr.mode = NORMAL_CALL;
  vm->cc = NULL;
  vm->closure = NULL;
}

void vm_init (vm_t vm)
{
  os_memset (vm, 0, sizeof (struct LambdaVM));
  vm_init_environment (vm);
  vm->code = NULL;
  vm->stack = (u8_t *)os_malloc (GLOBAL_REF (VM_STKSEG_SIZE));
  vm->globals = NULL;
}

void vm_clean (vm_t vm)
{
  /* NOTE: vm->code will be free in LEF */

  os_free (vm->stack);
  vm->stack = NULL;

  os_free (vm->globals);
  vm->globals = NULL;

  clean_symbol_table ();
  os_free (vm);
  vm = NULL;
}

void vm_init_globals (vm_t vm, lef_t lef)
{
  u8_t *stack = vm->stack; // backup stack
  u8_t *code = vm->code;   // backup code seg
  /* NOTE: We have to create a temporary global code in gsize, and set it to
   *       vm->code, since the size of code_seg may be smaller than data_seg.
   */
  vm->code = LEF_GLOBAL (lef);
  vm->pc = 0;
  vm->state = VM_INIT_GLOBALS;

  vm_run (vm);
  size_t size = vm->sp;
  vm->globals = (object_t)os_malloc (size);
  os_memcpy (vm->globals, vm->stack, size);
  vm->code = code; // restore code segment

  /* #ifdef LAMBDACHIP_DEBUG */
  /*   os_printk ("Globals %d: sp: %d\n", vm->sp / sizeof (Object), vm->sp); */
  /*   for (u32_t i = 0; i < vm->sp / sizeof (Object); i++) */
  /*     { */
  /*       object_t o = &GLOBAL (i); */
  /*       os_printk ("global %d: %p\n", i, o); */
  /*       if (o == NULL) */
  /*         { */
  /*           // placeholder */
  /*           os_printk ("placeholder 0\n"); */
  /*         } */
  /*       else */
  /*         { */
  /*           object_printer (o); */
  /*           os_printk ("\n"); */
  /*         } */
  /*     } */
  /* #endif */

  vm_init_environment (vm);
}

void vm_load_lef (vm_t vm, lef_t lef)
{
  GLOBAL_SET (VM_CODESEG_SIZE, lef->psize);
  GLOBAL_SET (VM_GLOBALSEG_SIZE, lef->gsize);

  create_symbol_table (&lef->symtab);
  // FIXME: not all mem section is data seg

  vm_init_globals (vm, lef);

  vm->code = LEF_PROG (lef);
  vm->pc = lef->entry;
}

void vm_restart (vm_t vm)
{
  GLOBAL_SET (VM_CODESEG_SIZE, 0);
  GLOBAL_SET (VM_DATASEG_SIZE, 0);
  GLOBAL_SET (VM_GLOBALSEG_SIZE, 0);

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
  else if (DOUBLE_ENCODE (bytecode))
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
  PANIC ("BUG in pre_fetch: it's impossible to be here!");
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
        PANIC ("vm_run panic!\n");
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
      /* u32_t bound */
      /*   = vm->sp ? (vm->sp - (vm->fp == NO_PREV_FP ? 0 : vm->fp + FPS)) : 0;
       */

      // for (u32_t i = 0; i < bound / 8; i++)
      /* for (u32_t i = 0; i < 8; i++) */
      /*   { */
      /*     object_t obj = (object_t)LOCAL_FIX (i); */
      /*     os_printk ( */
      /*       "obj: local = %d, offset=%d, local_sp=%d, type = %d, value =" */
      /*       "% d\n ", */
      /*       vm->local, i, vm->fp + FPS + i * 8, obj->attr.type, */
      /*       (imm_int_t)obj->value); */
      /*   } */
      /* os_printk ("------------END-----------\n"); */
      /* getchar (); */

      /* printf ("TOS: "); */
      /* object_printer (&TOP_OBJ ()); */
      /* printf ("\n"); */

      if (0 == vm->sp)
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
      /* os_printk ("pc: %d, local: %d, sp: %d, fp: %d, ppc: %d\n", vm->pc, */
      /*            vm->local, vm->sp, vm->fp, *((reg_t *)(vm->stack +
       * vm->fp))); */
      /* os_printk ("----------LOCAL------------\n"); */
      /* u32_t bound = (vm->sp - (vm->fp == NO_PREV_FP ? 0 : vm->fp + FPS)); */
      /* for (u32_t i = 0; i < bound / 8; i++) */
      /*   { */
      /*     object_t obj = (object_t)LOCAL_FIX (i); */
      /*     os_printk ("obj: offset = %d, type = %d, value = %d\n", */
      /*                vm->local + i * 8, obj->attr.type,
       * (imm_int_t)obj->value); */
      /*   } */
      /* os_printk ("------------END-----------\n"); */
      /* getchar (); */
    }

  // FIXME: optimize it to reduce redundant copying
  if (ret)
    {
      *ret = POP_OBJ ();
      /* NOTE: Since we use the deref tick here for copying,
       *       we must reset gc to 1 again!!! */
      ret->attr.gc = GEN_1_OBJ;

      if (VM_EXCPT_CONT == vm->state)
        {
          PUSH_REG (vm->pc);
          PUSH_REG (vm->sp);
        }
    }
  else
    {
      POP_OBJ ();
    }
}
