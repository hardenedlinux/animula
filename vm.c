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

static inline closure_t capture_closure(vm_t vm)
{
  return NULL;
}

static inline void jump_closure(vm_t vm, closure_t closure)
{
  /* Jump will not save the current environment */
  u16_t* env = (u16_t*)ss_read_u32(closure->env);
  /* The first byte of env is its own size */
  os_memcpy(vm->stack, env + 1, *env);
  vm->pc = closure->entry;
}

// Return value should push to stack
static inline void call_proc(vm_t vm, closure_t proc)
{
  object_t obj = make_continuation();
  cont_t cont = (cont_t)obj->value;

  /* FIXME:
   * 1. It's too slow to store cc to heap for each call, the better way is to
   *    store it when necessary. This optimizing may require new instruction.
   * 2. This caveat may increase GC frequency.
   */
  u16_t parent = ss_store_u32((u32_t)vm->cc);

  cont->parent = parent;
  cont->closure = ss_store_u32((u32_t)proc);
  vm->cc = cont;
  vm->pc = proc->entry;
}

static inline void generate_object(vm_t vm, object_t obj)
{
  bytecode8_t bc;
  bc.all = NEXT_DATA();
  obj->attr.gc = 0;
  obj->attr.type = bc.all;

  switch(obj->attr.type)
    {
    case imm_int:
      {
        u8_t value[4] = {0};
#if defined LAMBDACHIP_BIG_ENDIAN
        value[0] = NEXT_DATA();
        value[1] = NEXT_DATA();
        value[2] = NEXT_DATA();
        value[3] = NEXT_DATA();
#else
        value[3] = NEXT_DATA();
        value[2] = NEXT_DATA();
        value[1] = NEXT_DATA();
        value[0] = NEXT_DATA();
#endif
        VM_DEBUG("(general-object-encode %d)\n", *((s32_t*)value));
        obj->value = (void*)value;
        break;
      }
    case string:
      {
        const char* str = (char*)(vm->code + vm->pc);
        size_t size = strnlen(str, MAX_STR_LEN) + 1;
        vm->pc += size;
        /* char* buf = (char*)os_malloc(size); */
        /* memcpy(buf, str, size); */
        VM_DEBUG("(push-string-object \"%s\")\n", str);
        obj->value = (void*)str;
        break;
      }
    default:
      {
        os_printk("Oops, invalid object %d!\n", bc.type);
        VM_PANIC();
      }
    }
}

static inline void interp_single_encode(vm_t vm, bytecode8_t bc)
{
  switch(bc.type)
    {
    case PUSH_SMALL_CONST:
      {
        /* Push bc.data to the current stack top */
        VM_DEBUG("(push-4bit-const %d)\n", bc.data);
        PUSH(bc.data);
        break;
      }
    case LOAD_SS_SMALL:
      {
        /* Load small offset from static stack, 0<= offset <=31 */
        VM_DEBUG("(ss-load-4bit-const %d)\n", bc.data);
        PUSH_FROM_SS(bc);
        break;
      }
    case PUSH_GLOBAL:
      {
        u8_t global = global_get(bc.data);
        VM_DEBUG("(global-ref %d)\n", global);
        PUSH(global);
        break;
      }
    case SET_GLOBAL:
      {
        VM_DEBUG("(global-set! %d %d)\n", bc.data, TOP());
        global_set(bc.data, POP());
        break;
      }
    case CALL_CLOSURE:
      {
        closure_t closure = (closure_t)ss_read_u32(POP());
        VM_DEBUG("(call-closure 0x%p)\n", closure);
        // bc.data is the number of arguments
        SAVE_ENV();
        HANDLE_ARITY(bc.data);
        jump_closure(vm, closure);
        break;
      }
    case JUMP_CLOSURE:
      {
        closure_t closure = (closure_t)ss_read_u32(POP());
        VM_DEBUG("(jump-closure 0x%p)\n", closure);
        // bc.data is the number of arguments
        HANDLE_ARITY(bc.data);
        jump_closure(vm, closure);
        break;
      }
    case JUMP:
      {
        VM_DEBUG("(jump 0x%x)\n", bc.data);
        // relative jump, bc.data is the offset
        vm->pc += bc.data;
        break;
      }
    case JUMP_FALSE:
      {
        object_t obj = (object_t)ss_read_u32(TOP());
        VM_DEBUG("(jump-tos-false 0x%x 0x%p)\n", bc.data, obj);
        if(object_is_false(obj))
          {
            vm->sp++;
            vm->pc += bc.data;
          }
        break;
      }
    default:
      {
        os_printk("Invalid bytecode %X\n", bc.all);
        panic("interp_single_encode panic!\n");
      }
    }
}

static inline void interp_double_encode(vm_t vm, bytecode16_t bc)
{
  switch(bc.type)
    {
    case PUSH_8BIT_CONST:
      {
        VM_DEBUG("(push-8bit-const %d)\n", bc.data);
        PUSH(bc.data);
        break;
      }
    case LONG_JUMP:
      {
        VM_DEBUG("(long-jump 0x%x)\n", bc.data);
        u16_t offset = ss_read_u16(bc.data) + 128;
        vm->pc += offset;
        break;
      }
    case LONG_JUMP_TOS:
      {
        object_t obj = (object_t)ss_read_u32(TOP());
        VM_DEBUG("(long-jump-tos-false 0x%x 0x%p)\n", bc.data, obj);
        if(object_is_false(obj))
          {
            u16_t offset = ss_read_u16(bc.data) + 128;
            vm->sp++;
            vm->pc += offset;
          }
        break;
      }
    case MAKE_CLOSURE:
      {
        u32_t entry = ss_read_u32(bc.data);
        size_t size = vm->sp - vm->fp;
        u8_t *context = (u8_t*)gc_malloc(size + 1);
        /* The first byte of env is its own size */
        *context = size;
        os_memcpy(context + 1, vm->stack + vm->fp, size);
        u16_t env = ss_store_u32((u32_t)context);
        object_t obj = make_closure(env, entry);
        VM_DEBUG("(make-closure 0x%p)\n", (void*)entry);
        u8_t offset = ss_store_tiny_encode((u32_t)obj);

        if (offset > 0b1111)
          {
            VM_DEBUG("Encoded tiny encoder is larger than %d\n", offset);
            panic("BUG: Invalid tiny encode!\n");
          }

        u8_t closure = 0xf || (closure & 0xf);
        PUSH(closure);
        break;
      }
    default:
      ;
    };
}

static inline void interp_triple_encode(vm_t vm, bytecode24_t bc)
{
  switch(bc.type)
    {
    case CALL_PROC:
      {
        closure_t proc = (closure_t)ss_read_u32(bc.bc3);
        VM_DEBUG("(call-proc %d 0x%p)\n", bc.bc2, proc);
        SAVE_ENV();
        HANDLE_ARITY(bc.bc2);
        call_proc(vm, proc);
        break;
      }
    case PUSH_16BIT_CONST:
      {
        VM_DEBUG("(push-16bit-const %d)\n", bc.data);
        PUSH(bc.data);
        break;
      }
    case VEC_REF:
      {
        vec_t vec = (vec_t)ss_read_u32(bc.bc2);
        VM_DEBUG("(vec-ref 0x%p %d)\n", vec, bc.bc3);
        PUSH(vector_ref(vec, bc.bc2));
        break;
      }
    default:
      break;
    }
}

static inline void interp_quadruple_encode(vm_t vm, bytecode32_t bc)
{
  switch(bc.type)
    {
    case VEC_SET:
      {
        vec_t vec = (vec_t)ss_read_u32(bc.bc1);
        object_t obj = (object_t)ss_read_u32(bc.bc3);
        VM_DEBUG("(vec-set! 0x%p %d 0x%p)\n", vec, bc.bc2, obj);
        vector_set(vec, bc.bc2, obj);
      }
    default:
      break;
    }
}

static inline void call_prim(vm_t vm, pn_t pn)
{
  prim_t prim = get_prim(pn);

  switch(pn)
    {
    case int_add:
    case int_sub:
    case int_mul:
    case int_div:
      {
        ARITH_PRIM();
        break;
      }
    case object_print:
      {
        printer_prim_t fn = (printer_prim_t)prim->fn;
        Object obj = POPx(Object);
        fn(&obj);
        break;
      }
    default:
      os_printk("Invalid prim number: %d\n", pn);
    }
}

static inline void interp_special(vm_t vm, bytecode8_t bc)
{
  switch(bc.type)
    {
    case PRIMITIVE:
      {
        VM_DEBUG("(primitive %d %s)\n", bc.data, prim_name(bc.data));
        call_prim(vm, (pn_t)bc.data);
        VM_DEBUG("result: %d\n", TOP());
        break;
      }
    case OBJECT:
      {
        switch(bc.data)
          {
          case GENERAL_OBJECT:
            {
              Object obj;
              generate_object(vm, &obj);
              PUSHx(Object, obj);
              break;
            }
          case FALSE:
            {
              VM_DEBUG("(push-boolean-false)\n");
              PUSH(bc.all);
              break;
            }
          case TRUE:
            {
              VM_DEBUG("(push-boolean-true)\n");
              PUSH(bc.all);
              break;
            }
          }
        break;
      }
    default:
      {
        if (HALT == bc.all)
          vm->state = VM_STOP;
        VM_DEBUG("Halt here!\n");
      }
    }
}

static inline bytecode8_t fetch_next_bytecode(vm_t vm)
{
  bytecode8_t bc;

  if(vm->pc < VM_CODESEG_SIZE)
    {
      bc.all = vm->code[vm->pc++];
      //os_printk("BC type(%x) data(%x)\n", bc.type, bc.data);
    }
  else
    {
      os_printk("Oops, no more bytecode!\n");
      VM_PANIC();
    }

  return bc;
}

void vm_init(vm_t vm)
{
  os_memset(vm, 0, sizeof(struct LambdaVM));
  vm->fetch_next_bytecode = fetch_next_bytecode;
  vm->state = VM_RUN;
  vm->cc = NULL;
  vm->sp = 0;
  vm->code = (u8_t*)os_malloc(VM_CODESEG_SIZE);
  vm->stack = (u8_t*)os_malloc(VM_STKSEG_SIZE);
  /* FIXME: We set it to 256, it should be decided by the end of ss in LEF
   */
  __store_offset = 256;
}

void vm_load_lef(vm_t vm, lef_t lef)
{
  os_memcpy(vm->code, LEF_PROG(lef), lef->psize);
  vm->data = (u8_t*)os_malloc(lef->msize);
  os_memcpy(vm->data, LEF_MEM(lef), lef->msize);
}

void vm_restart(vm_t vm)
{
  /* TODO:
   * 1. Free all objects in the heap
   * 2. Clean all global information, include ss
   */

  vm_init(vm);
}

static inline encode_t pre_fetch(vm_t vm, bytecode8_t bytecode)
{
  //  VM_DEBUG("Prefetch: %x\n", bytecode.all);
  if(SINGLE_ENCODE(bytecode))
    {
      return SINGLE;
    }
  if(DOUBLE_ENCODE(bytecode))
    {
      return DOUBLE;
    }
  else if(TRIPLE_ENCODE(bytecode))
    {
      return TRIPLE;
    }
  else if(QUADRUPLE_ENCODE(bytecode))
    {
      return QUADRUPLE;
    }
  else if(IS_SPECIAL(bytecode))
    {
      return SPECIAL;
    }

  VM_DEBUG("Invalid encode %x\n", bytecode.type);
  panic("BUG in pre_fetch: it's impossible to be here!");
  return -1;
}

static inline void dispatch(vm_t vm, bytecode8_t bc)
{
  switch(pre_fetch(vm, bc))
    {
    case SINGLE:
      {
        interp_single_encode(vm, bc);
        break;
      }
    case DOUBLE:
      {
        bytecode16_t bc16;
        bc16.bc1 = bc.all;
        bc16.bc2 = NEXT_DATA();
        interp_double_encode(vm, bc16);
        break;
      }
    case TRIPLE:
      {
        bytecode24_t bc24;
        bc24.bc1 = bc.all;
        bc24.bc2 = NEXT_DATA();
        bc24.bc3 = NEXT_DATA();
        interp_triple_encode(vm, bc24);
        break;
      }
    case QUADRUPLE:
      {
        bytecode32_t bc32;
        bc32.bc1 = bc.all;
        bc32.bc2 = NEXT_DATA();
        bc32.bc3 = NEXT_DATA();
        bc32.bc4 = NEXT_DATA();
        interp_quadruple_encode(vm, bc32);
        break;
      }
    case SPECIAL:
      interp_special(vm, bc);
      break;
    default:
      {
        os_printk("Invalid bytecode type!\n");
        panic("vm_run panic!\n");
      }
    };
}

void vm_load_compiled_file(const char *filename)
{
  os_printk("Lambdachip hasn't supported file loading yet!\n");
}

void vm_run(vm_t vm)
{
  VM_DEBUG("VM run!\n");

  while(VM_RUN == vm->state)
    {
      /* TODO:
       * 1. Add debug info
       */
      dispatch(vm, FETCH_NEXT_BYTECODE());
    }
}
