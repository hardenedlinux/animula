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

#include "gc.h"

/* The GC in LambdaChip is "object-based generational GC".
   We don't perform mark/sweep, or any reference counting.

   The meaning of `gc' field in Object:
   * 1~3 means the generation, 0 means free.
   * The `gc' will increase by 1 when it survives from GC.
   * For stack-allocated object, `gc' field is always 0.

 */

static RB_HEAD (ActiveRoot, ActiveRootNode)
  ActiveRootHead = RB_INITIALIZER (&ActiveRootHead);

RB_GENERATE_STATIC (ActiveRoot, ActiveRootNode, entry, active_root_compare);

static obj_list_head_t obj_free_list;
static obj_list_head_t obj_list_free_list;
static obj_list_head_t list_free_list;
static obj_list_head_t vector_free_list;
static obj_list_head_t pair_free_list;
static obj_list_head_t closure_free_list;

void gc_init (void)
{
  SLIST_INIT (&obj_free_list);
  SLIST_INIT (&obj_list_free_list);
  SLIST_INIT (&list_free_list);
  SLIST_INIT (&vector_free_list);
  SLIST_INIT (&pair_free_list);
  SLIST_INIT (&closure_free_list);
}

static void free_object (object_t obj)
{
  switch (obj->attr.type)
    {
    case pair:
      {
        os_free ((void *)((pair_t)obj->value)->car);
        os_free ((void *)((pair_t)obj->value)->cdr);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_t prev = NULL;
        obj_list_head_t *head = (obj_list_head_t *)obj->value;

        SLIST_FOREACH (node, head, next)
        {
          os_free ((void *)prev);
          prev = node;
          os_free ((void *)node->obj);
        }

        os_free (prev); // free the last node
        os_free (obj->value);
        break;
      }
    case symbol:
    case continuation:
    case string:
      {
        os_free ((void *)obj->value);
        break;
      }
    default:
      break;
    }
  /* We should set value to NULL here, since obj is not guarrenteed to be
   * freed by GC, and it could be recycled by the pool.
   */
  os_free ((void *)obj);
  obj = NULL;
}

static void recycle_object (gobj_t type, object_t obj)
{
  obj_list_t node = NULL;

  switch (type)
    {
    case gc_object:
      {
        RECYCLE_OBJ (obj_free_list);
        break;
      }
    case gc_list:
      {
        RECYCLE_OBJ (list_free_list);
        break;
      }
    case gc_pair:
      {
        RECYCLE_OBJ (pair_free_list);
        break;
      }
    case gc_vector:
      {
        RECYCLE_OBJ (vector_free_list);
        break;
      }
    case gc_closure:
      {
        RECYCLE_OBJ (closure_free_list);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", type);
        panic ("recycle_object is down!");
      }
    }
}

static inline __insert (ActiveRootNode *an)
{
  RB_INSERT (ActiveRoot, &ActiveRootHead, an);
}

static void active_root_insert (object_t obj)
{
  ActiveRootNode *an = (ActiveRootNode *)os_malloc (sizeof (ActiveRootNode));

  if (!an)
    {
      panic ("GC: We're doomed! There're even no RAMs for GC!\n");
    }

  an->value = (void *)value;

  switch (obj->type)
    {
    case imm_int:
      {
        break;
      }
    case pair:
      {
        pair_t p = (pair_t)obj->value;
        active_root_insert (p->car);
        active_root_insert (p->cdr);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = (obj_list_head_t *)obj->value;

        SLIST_FOREACH (node, head, next)
        {
          active_root_insert (node->obj);
        }
        break;
      }
    case vector:
      {
        panic ("GC: Hey, did we support Vector now? If so, please fix me!\n");
      }
    default:
      {
        ActiveRootNode *van
          = (ActiveRootNode *)os_malloc (sizeof (ActiveRootNode));

        if (!van)
          {
            panic ("GC: We're doomed! There're even no RAMs for GC!\n");
          }

        van->value = obj->value;
      }
    }

  __insert (an);
}

static void static void active_root_insert_frame (u8_t *stack, u32_t local,
                                                  u8_t cnt)
{
  for (u8_t i = 0; i < cnt; local += sizeof (Object))
    {
      object_t obj = (objec_t *)(stack + local);

      if (!obj)
        panic ("active_root_insert_frame: Invalid object address!");

      active_root_insert (obj);
    }
}

static void build_active_root (const gc_info_t gci)
{
  // 1. Count frames and get each fp
  // 2. Generate Active Root Tree

  u8_t *stack = gci->stack;

  for (u32_t fp = gci->fp, u32_t sp = gci->sp; fp; sp = fp, fp = NEXT_FP ())
    {
      u32_t local = fp + FPS;
      u8_t obj_cnt = (sp - local) / sizeof (Object);
      active_root_insert_frame (stack, local, obj_cnt);
    }
}

static void clean_active_root ()
{
  ActiveRootNode *node = NULL;
  ActiveRootNode *prev = NULL;

  RB_FOREACH (node, ActiveRoot, &ActiveRootHead)
  {
    os_free ((void *)prev);
    prev = node;
  }

  os_free ((void *)prev);
}

bool gc (const gc_info_t gci)
{
  /* TODO:
   * 1. Obj pool is empty, goto 3
   * 2. Free all unused obj:
   *    a. move from ref_list to free_list (obj pool)
   *    b. if no collectable obj, then goto 3
   * 3. Free obj pool
   */

  build_active_root (gci);

  // TODO: gc

  clean_active_root ();
  return false;
}

void gc_book (gobj_t type, object_t obj)
{

  obj_list_t node = (obj_list_t)gc_malloc (sizeof (ObjectList));

  node->obj = obj;

  switch (type)
    {
    case gc_object:
      {
        SLIST_INSERT_HEAD (&obj_free_list, node, next);
        break;
      }
    case gc_list:
      {
        SLIST_INSERT_HEAD (&list_free_list, node, next);
        break;
      }
    case gc_pair:
      {
        SLIST_INSERT_HEAD (&pair_free_list, node, next);
        break;
      }
    case gc_vector:
      {
        SLIST_INSERT_HEAD (&vector_free_list, node, next);
        break;
      }
    case gc_closure:
      {
        SLIST_INSERT_HEAD (&closure_free_list, node, next);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", type);
        panic ("gc_book is down!");
      }
    }
}

void *gc_pool_malloc (gobj_t type)
{
  /* NOTE:
   * Object pool design is based on the facts:
   *    0. The first choice is gc_pool_malloc
   *    1. VM only allocates objects with gc_malloc
   *    2. All objects are well defined and fixed sized
   *    3. All objects are recycleable in runtime
   * That's why gc_pool_malloc is useful here.
   */

  /* NOTE: If object was freed, then the internal obj was freed, so we don't
   *       have to maintain `gc' fields in the internal obj.
   */
  void *ret = NULL;

  switch (type)
    {
    case gc_object:
      {
        MALLOC_OBJ_FROM_POOL (obj_free_list);
        break;
      }
    case gc_list:
      {
        MALLOC_OBJ_FROM_POOL (pair_free_list);
        break;
      }
    case gc_pair:
      {
        MALLOC_OBJ_FROM_POOL (pair_free_list);
        break;
      }
    case gc_vector:
      {
        MALLOC_OBJ_FROM_POOL (vector_free_list);
        break;
      }
    case gc_closure:
      {
        MALLOC_OBJ_FROM_POOL (closure_free_list);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", type);
        panic ("gc_pool_malloc is down!");
      }
    }

  return ret;
}

void *gc_malloc (size_t size)
{
  do
    {
      /* NOTE:
       * 1. gc_malloc will allocate from free_list (obj pool) first
       * 2. Call malloc if a or b meets:
       *    a. obj pool is empty
       *    b. obj pool has no suitable obj
       * 3. malloc is failed:
       *    a. collect the whole obj pool
       *    b. malloc again
       *    c. malloc is still failed, then waiting and printing error
       */

      void *ptr = os_malloc (size);
      if (ptr)
        return ptr;

      gc ();
    }
  while (1);
}

// gc_free shouldn't be called explicitly, it should be only used by GC
void gc_free (void *ptr)
{
  // TODO: recycle to pool, if pool size exceeds to max, then call low-level
  // free.
}
