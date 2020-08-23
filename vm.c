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

#include "vm.h"

static void call_prim (vm_t vm, pn_t pn)
{
  prim_t prim = get_prim (pn);

  switch (pn)
    {
    case ret:
      {
        //        printf ("ret sp: %d, fp: %d, pc: %d\n", vm->sp, vm->fp,
        //        vm->pc);
        for (int i = 0; i < 2; i++)
          {
            RESTORE ();
            //  printf ("after sp: %d, fp: %d, pc: %d\n", vm->sp, vm->fp,
            //  vm->pc);
          }
        break;
      }
    case int_add:
    case int_sub:
    case int_mul:
    case int_div:
    case int_modulo:
    case int_remainder:
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
    case int_eq:
      {
        arith_prim_t fn = (arith_prim_t)prim->fn;
        size_t size = sizeof (struct Object);
        Object x = POP_OBJ ();
        Object y = POP_OBJ ();
        Object z = {.attr = {.type = boolean, .gc = 0}, .value = NULL};
        z.value = (void *)fn ((imm_int_t)y.value, (imm_int_t)x.value);
        PUSH_OBJ (z);
        break;
      }
    case pop:
      {
        if (vm->sp)
          POP_OBJ ();
        break;
      }
    default:
      os_printk ("Invalid prim number: %d\n", pn);
    }
}

static uintptr_t vm_get_uintptr (vm_t vm, u8_t *buf)
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

static object_t generate_object (vm_t vm, object_t obj)
{
  bytecode8_t bc;
  bc.all = NEXT_DATA ();
  obj->attr.gc = 0;
  obj->attr.type = bc.all;

  switch (obj->attr.type)
    {
    case imm_int:
      {
        u8_t buf[4] = {0};
        imm_int_t value = vm_get_uintptr (vm, buf);
        VM_DEBUG ("(push-integer-object %d)\n", value);
        obj->value = (void *)value;
        break;
      }
    case string:
      {
        const char *str = (char *)(vm->code + vm->pc);
        vm->pc += strnlen (str, MAX_STR_LEN) + 1;
        VM_DEBUG ("(push-string-object \"%s\")\n", str);
        obj->value = (void *)str;
        break;
      }
    case procedure:
      {
        u8_t buf[sizeof (uintptr_t)] = {0};
        u32_t offset = vm_get_uintptr (vm, buf);
        VM_DEBUG ("(push-proc-object 0x%x)\n", offset);
        obj->value = (void *)offset;
        break;
      }
    case primitive:
      {
        u8_t buf[sizeof (uintptr_t)] = {0};
        uintptr_t prim = vm_get_uintptr (vm, buf);
        // vm->pc += sizeof (uintptr_t);
        VM_DEBUG ("(push-prim-object %d %s)\n", prim, prim_name (prim));
        obj->value = (void *)prim;
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
        /* printf ("obj: type = %d, value = %d\n", obj->attr.type, */
        /*         (imm_int_t)obj->value); */
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
        CALL (obj);
        break;
      }
    case CALL_LOCAL_EXTEND:
      {
        VM_DEBUG ("(call-local %d)\n", bc.data + 16);
        object_t obj = (object_t)LOCAL (bc.data + 16);
        CALL (obj);
        break;
      }
    case FREE_REF:
      {
        u8_t frame = NEXT_DATA ();
        u8_t up = (frame & 0b001111);
        u8_t offset = ((bc.data << 2) | ((frame & 0b110000) >> 4));
        VM_DEBUG ("(free %x %d)\n", up, offset);
        object_t obj = (object_t)FREE_VAR (up, offset);
        /* printf ("obj: type = %d, value = %d\n", obj->attr.type, */
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
        u8_t up = (frame & 0b001111);
        u8_t offset = ((bc.data << 2) | ((frame & 0b110000) >> 4));
        VM_DEBUG ("(call-free %x %d)\n", up, offset);
        object_t obj = (object_t)FREE_VAR (up, offset);
        CALL (obj);
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
        vec_t vec = (vec_t)ss_read_u32 (bc.bc2);
        VM_DEBUG ("(vec-ref 0x%p %d)\n", vec, bc.bc3);
        PUSH (vector_ref (vec, bc.bc2));
        break;
      }
    case CALL_PROC:
      {
        u32_t offset = bc.data;
        VM_DEBUG ("(call-proc 0x%x)\n", offset);
        PROC_CALL (offset);
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
        vec_t vec = (vec_t)ss_read_u32 (bc.bc1);
        object_t obj = (object_t)ss_read_u32 (bc.bc3);
        VM_DEBUG ("(vec-set! 0x%p %d 0x%p)\n", vec, bc.bc2, obj);
        vector_set (vec, bc.bc2, obj);
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
        closure_t closure = make_closure (arity, size, entry);
        Object obj = {.attr = {.type = closure_on_heap, .gc = 0},
                      .value = (void *)closure};
        for (u8_t i = size; i > 0; i--)
          {
            closure->env[i - 1] = POP_OBJ ();
            /* printf ("closure->env[%d]: type = %d, value = %d\n", i - 1, */
            /*         closure->env[i - 1].attr.type, */
            /*         (imm_int_t) (closure->env[i - 1].value)); */
          }
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
        VM_DEBUG ("result: %d\n", (s32_t)TOP_OBJ ().value);
        break;
      }
    case OBJECT:
      {
        switch (bc.data)
          {
          case GENERAL_OBJECT:
            {
              Object obj;
              generate_object (vm, &obj);
              PUSH_OBJ (obj);
              // object_t o = (object_t)LOCAL (0);
              /* Object b = TOP_OBJ (); */
              /* object_t o = (object_t)LOCAL (0); */
              /* printf ("%d === %d === %d\n", obj.attr.type, b.attr.type, */
              /*         o->attr.type); */
              /* printf ("vm->stack (%p) + vm->sp (%d) - size (%d)= %p\n", */
              /*         vm->stack, vm->sp, sizeof (Object), */
              /*         vm->stack + vm->sp - sizeof (Object)); */
              /* printf ("vm->stack (%p) + vm->fp (%d) + 2 + offset (%d) = %p\n
               * ", */
              /*         vm->stack, vm->fp, 0, LOCAL (0)); */

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
          }
        break;
      }
    case CONTROL:
      {
        switch (bc.data)
          {
          case HALT:
            {
              vm->state = VM_STOP;
              VM_DEBUG ("Halt here!\n");
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
  bytecode8_t bc;

  if (vm->pc < GLOBAL_REF (VM_CODESEG_SIZE))
    {
      bc.all = vm->code[vm->pc++];
      // os_printk("BC type(%x) data(%x)\n", bc.type, bc.data);
    }
  else
    {
      os_printk ("Oops, no more bytecode!\n");
      VM_PANIC ();
    }

  return bc;
}

void vm_init (vm_t vm)
{
  os_memset (vm, 0, sizeof (struct LambdaVM));
  vm->fetch_next_bytecode = fetch_next_bytecode;
  vm->state = VM_RUN;
  vm->sp = 0;
  vm->fp = 0;
  vm->local = 0;
  vm->shadow = 0;
  vm->cc = NULL;
  vm->code = (u8_t *)os_malloc (GLOBAL_REF (VM_CODESEG_SIZE));
  vm->data = (u8_t *)os_malloc (GLOBAL_REF (VM_DATASEG_SIZE));
  vm->stack = (u8_t *)os_malloc (GLOBAL_REF (VM_STKSEG_SIZE));
  vm->closure = NULL;
  /* FIXME: We set it to 256, it should be decided by the end of ss in LEF
   */
  __store_offset = 256;
}

void vm_clean (vm_t vm)
{
  os_free (vm->code);
  vm->code = NULL;

  os_free (vm->data);
  vm->data = NULL;

  os_free (vm->stack);
  vm->stack = NULL;
}

void vm_load_lef (vm_t vm, lef_t lef)
{
  os_memcpy (vm->code, LEF_PROG (lef), lef->psize);
  vm->data = (u8_t *)os_malloc (lef->msize);
  // FIXME: not all mem section is data seg
  os_memcpy (vm->data, LEF_MEM (lef), lef->msize);
  vm->pc = lef->entry;
}

void vm_restart (vm_t vm)
{
  /* TODO:
   * 1. Free all objects in the heap
   * 2. Clean all global information, include ss
   */

  vm_init (vm);
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

  while (VM_RUN == vm->state)
    {
      /* TODO:
       * 1. Add debug info
       */
      dispatch (vm, FETCH_NEXT_BYTECODE ());
      /* printf ("pc: %d, local: %d, sp: %d, fp: %d\n", vm->pc, vm->local,
       * vm->sp, */
      /*         vm->fp); */
      /* printf ("----------LOCAL------------\n"); */
      /* u32_t bound = (vm->sp - (vm->fp ? vm->fp + FPS : 0)); */
      /* for (u32_t i = 0; i < bound / 8; i++) */
      /*   { */
      /*     object_t obj = (object_t)LOCAL_FIX (i); */
      /*     printf ("obj: local = %d, type = %d, value = %d\n", vm->local + i *
       * 8, */
      /*             obj->attr.type, (imm_int_t)obj->value); */
      /*   } */
      /* printf ("------------END-----------\n"); */
      /* getchar (); */
    }
}
