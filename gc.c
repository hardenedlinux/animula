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

#include "gc.h"
#include "list.h"

#ifdef LAMBDACHIP_LINUX
#  include <sys/time.h>

#endif

int gc_final = 0;

static int get_gc_from_node (otype_t type, void *value);
/* The GC in LambdaChip is "object-based generational GC".
   We don't perform mark/sweep, or any reference counting.

   The meaning of `gc' field in Object:
   * 3 means permarnent.
   * 1~2 means the generation, 0 means free.
   * The `gc' will increase by 1 when it survives from GC.
   * For stack-allocated object, `gc' field is always 0.

 */

static RB_HEAD (ActiveRoot, ActiveRootNode)
  ActiveRootHead = RB_INITIALIZER (&ActiveRootHead);

RB_GENERATE_STATIC (ActiveRoot, ActiveRootNode, entry, active_root_compare);

static obj_list_head_t pair_free_pool;
static obj_list_head_t vector_free_pool;
static obj_list_head_t list_free_pool;
static obj_list_head_t closure_free_pool;
static obj_list_head_t obj_free_pool;

static struct Pre_ARN _arn = {0};
struct Pre_OLN _oln = {0};

static void pre_allocate_active_nodes (void)
{
  for (int i = 0; i < PRE_ARN; i++)
    {
      _arn.arn[i] = (ActiveRootNode *)os_malloc (sizeof (ActiveRootNode));

      if (NULL == _arn.arn[i])
        {
          os_printk ("GC: We're doomed! Did you set a too large PRE_ARN?");
          PANIC ("Try to set PRE_ARN smaller!");
        }
    }

  _arn.index = 0;
  VM_DEBUG ("PRE_ARN: %d, pre-allocate %d bytes.\n", PRE_ARN,
            PRE_ARN * sizeof (ActiveRootNode));
}

static ActiveRootNode *arn_alloc (void)
{
  if (PRE_ARN == _arn.index)
    {
      PANIC ("GC: We're doomed! Did you set a too small PRE_ARN?"
             "Try to set PRE_ARN larger!");
    }

  os_printk ("*arn_alloc, _arn.index = %d\n", _arn.index);
  return _arn.arn[_arn.index++];
}

static void object_list_node_pre_allocate (void)
{
  int i = 0;
  for (; i < PRE_OLN; i++)
    {
      obj_list_t ptr = (obj_list_t)os_malloc (sizeof (ObjectList));
      if (NULL == ptr)
        {
          PANIC ("GC: We're doomed! Did you set a too large PRE_OLN?"
                 "Try to set PRE_OLN smaller!");
        }
      else
        {
          _oln.oln[i] = ptr;
        }
    }

  _oln.index = 0;
  VM_DEBUG ("PRE_OLN: %d, cnt: %d, pre-allocate %d bytes.\n", PRE_OLN, i - 1,
            PRE_OLN * sizeof (ObjectList));
}

obj_list_t object_list_node_alloc (void)
{
  obj_list_t ret = NULL;

  if (!object_list_node_available ())
    {
      return NULL;
    }

  ret = _oln.oln[_oln.index];
  // do not delete the following line which worth $2000 USD at least
  _oln.oln[_oln.index] = (void *)0xDEAD0001;
  if (NULL == ret)
    {
      os_printk ("BUG: there's no obj_list node, but cnt is %d\n", _oln.index);
      PANIC ("Maybe it's not recycled correctly?");
    }
  _oln.index++;
  return ret;
}

// put obj_list_t back into OLN for future use
static void object_list_node_recycle (obj_list_t node)
{
  _oln.oln[--_oln.index] = node;
}

size_t object_list_node_available (void)
{
  return (PRE_OLN - _oln.index);
}

static void active_nodes_clean (void)
{
  for (int i = 0; i < PRE_ARN; i++)
    {
      os_free (_arn.arn[i]);
    }

  _arn.index = 0;
  os_printk ("ARN clean done!\n");
  VM_DEBUG ("ARN clean!\n");
}

static void object_list_node_clean (void)
{
  printf ("_oln.index = %d\n", _oln.index);
  // do not modify i to start from 0, which will cost you at least $2000 USD
  if (0 != _oln.index)
    {
      PANIC ("Not all nodes returned to OLN");
    }
  for (int i = _oln.index; i < PRE_OLN; i++)
    {
      printf ("i = %d, ", i);
      void *ptr = _oln.oln[i];
      if (NULL != ptr)
        {
          os_free (ptr);
          _oln.oln[i] = NULL;
        }
      else
        {
          PANIC ("Available OLN shall not be NULL\n");
        }
    }

  _oln.index = 0;
  os_printk ("OLN clean done!\n");
  VM_DEBUG ("OLN clean!\n");
}

static inline void insert (ActiveRootNode *an)
{
  RB_INSERT (ActiveRoot, &ActiveRootHead, an);
}

static inline bool exist (object_t obj)
{
  ActiveRootNode node = {.value = (void *)obj};
  return (NULL != RB_FIND (ActiveRoot, &ActiveRootHead, &node));
}

static void insert_value (void *value)
{
  ActiveRootNode *an = arn_alloc ();
  an->value = value;
  insert (an);
}

void free_object (object_t obj /* , bool force */)
{
  /* NOTE: Integers are self-contained object, so we can just release the object
   */
  // obj_list_t node = NULL;
  // bool node_exist = false;
  // SLIST_FOREACH (node, &list_free_pool, next)
  // {
  //   if (node->obj == obj)
  //     {
  //       node_exist = true;
  //       break;
  //     }
  // }

  // if (!node_exist)
  //   {
  //     return;
  //   }

  if (0xDEADBEEF == (uintptr_t)obj)
    {
      printf ("active_root_insert: oh a half list node!\n");
      printf ("let's skip it safely!\n");
      return;
    }

  if (!obj)
    {
      PANIC ("BUG: free a null object!");
    }

  u8_t gc = obj->attr.gc;
  if (gc_final)
    gc = FREE_OBJ;

  if (PERMANENT_OBJ == gc)
    return;

  switch (obj->attr.type)
    {
    case imm_int:
    case character:
    case real:
    case rational_pos:
    case rational_neg:
    case boolean:
    case null_obj:
    case none:
    case string:
    case symbol:
    case primitive:
    case procedure:
      {
        // simple object, we don't need to free its value
        // no need to free string
        // symbol should never be recycled
        break;
      }
    case pair:
      {
        free_object ((object_t) ((pair_t)obj->value)->car);
        free_object ((object_t) ((pair_t)obj->value)->cdr);

        free_object_from_pool (&pair_free_pool, obj->value);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);

        if (!SLIST_EMPTY (head))
          {
            node = SLIST_FIRST (head);
            while (node)
              {
                // call free_object recursively since node->obj can be a
                // composite object
                free_object (node->obj);
                // instead of free node, put node into OLN for future use
                SLIST_REMOVE_HEAD (head, next);
                os_free (node);
                node = SLIST_FIRST (head);
              }
          }

        free_object_from_pool (&list_free_pool, (object_t)obj->value);
        break;
      }
    case continuation:
    case mut_string:
      {
        os_free ((void *)obj->value);
        free_object_from_pool (&obj_free_pool, obj);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        free_object_from_pool (&closure_free_pool, obj->value);
        break;
      }
    default:
      {
        PANIC ("free_object: Invalid type %d!\n", obj->attr.type);
      }
    }

  free_object_from_pool (&obj_free_pool, obj);
}

void free_inner_object (otype_t type, void *value /* , bool force */)
{
  /* NOTE: Integers are self-contained object, so we can just release the object
   */
  if (!value)
    {
      PANIC ("BUG: free a null object!");
    }

  u8_t gc = get_gc_from_node (type, value);
  if (gc_final)
    gc = FREE_OBJ;
  if (PERMANENT_OBJ == gc)
    return;

  switch (type)
    {
    case pair:
      {
        free_object ((object_t) ((pair_t)value)->car);
        free_object ((object_t) ((pair_t)value)->cdr);
        free_object_from_pool (&pair_free_pool, value);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = &((list_t)value)->list;

        if (SLIST_EMPTY (head))
          {
            free_object_from_pool (&list_free_pool, value);
            break;
          }

        node = SLIST_FIRST (head);
        while (node)
          {
            // call free_object recursively since node->obj can be a composite
            // object
            free_object (node->obj);
            // instead of free node, put node into OLN for future use
            SLIST_REMOVE_HEAD (head, next);
            os_free (node);
            node = SLIST_FIRST (head);
          }

        free_object_from_pool (&list_free_pool, (object_t)value);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        free_object_from_pool (&closure_free_pool, value);
        break;
      }
    default:
      {
        PANIC ("free_inner_object: Invalid type %d!\n", type);
      }
    }
}

static void recycle_object (object_t obj)
{
  if (PERMANENT_OBJ == obj->attr.gc)
    return;

  switch (obj->attr.type)
    {
    case imm_int:
    case character:
    case real:
    case rational_pos:
    case rational_neg:
    case boolean:
    case null_obj:
    case none:
    case string:
    case symbol:
    case primitive:
    case procedure:
      {
        // These objects don't have to be recycled recursively.
        break;
      }
    case pair:
      {
        recycle_object (((pair_t)obj->value)->car);
        recycle_object (((pair_t)obj->value)->cdr);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = LIST_OBJECT_HEAD (obj);

        SLIST_FOREACH (node, head, next)
        {
          recycle_object (node->obj);
        }
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        free_object_from_pool (&closure_free_pool, obj);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", obj->attr.type);
        PANIC ("recycle_object is down!");
      }
    }

  obj->attr.gc = FREE_OBJ;
}

static void active_root_insert (object_t obj)
{
  if (0xDEADBEEF == (uintptr_t)obj)
    {
      return;
    }

  if (!obj)
    {
      PANIC ("BUG: active_root_insert - null obj!\n");
    }

  if (exist (obj))
    return;

  switch (obj->attr.type)
    {
    case pair:
      {
        pair_t p = (pair_t)obj->value;
        active_root_insert (p->car);
        active_root_insert (p->cdr);

        insert_value (obj->value);
        break;
      }
    case vector:
      {
        PANIC ("GC: Hey, did we support Vector now? If so, please fix me!\n");
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = &((list_t)obj->value)->list;

        SLIST_FOREACH (node, head, next)
        {
          active_root_insert (node->obj);
        }

        insert_value (obj->value);
        break;
      }
    default:
      {
        // Non-collection:

        /* procedure */
        /* closure_on_heap */
        /* closure_on_stack */
        /* mut_string */
        /* imm_int */
        /* primitive */
        /* string */
        break;
      }
    }

  insert_value ((void *)obj);
}

static void active_root_inner_insert (otype_t type, void *value)
{
  /* printf ("insert!\n"); */

  /* if (!value) */
  /*   printf ("active_root_inner_insert: oh null! %d\n", type); */

  /* if (exist (obj)) */
  /*   printf ("oh exist!\n"); */

  if (NULL == value)
    {
      // Some self-contain object may have NULL value
      return;
    }

  if (exist (value))
    return;

  switch (type)
    {
    case imm_int:
    case character:
    case real:
    case rational_pos:
    case rational_neg:
    case boolean:
    case null_obj:
    case none:
    case string:
    case symbol:
    case primitive:
    case procedure:
      {
        break;
      }
    case pair:
      {
        pair_t p = (pair_t)value;
        active_root_insert (p->car);
        active_root_insert (p->cdr);
        insert_value (value);
        break;
      }
    case vector:
      {
        PANIC ("GC: Hey, did we support Vector now? If so, please fix me!\n");
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_head_t *head = &((list_t)value)->list;

        SLIST_FOREACH (node, head, next)
        {
          active_root_insert (node->obj);
        }

        insert_value (value);
        break;
      }
    default:
      {
        PANIC ("BUG: active_root_inner_insert encountered a wrong type %d!\n",
               type);
        break;
      }
    }
}

static void active_root_insert_frame (const u8_t *stack, u32_t local, u8_t cnt)
{
  /* printf ("insert frame %d, %d\n", local, cnt); */
  /* getchar (); */
  for (u8_t i = 0; i < cnt; i++)
    {
      object_t obj = (object_t) (stack + local + i * sizeof (Object));

      if (!obj)
        PANIC ("active_root_insert_frame: Invalid object address!");

      active_root_inner_insert (obj->attr.type, obj->value);
    }
}

static void build_active_root (const gc_info_t gci)
{
  // 1. Count frames and get each fp
  // 2. Generate Active Root Tree

  u8_t *stack = gci->stack;
  reg_t fp = gci->fp;
  reg_t sp = gci->sp;
  bool run = true;

  for (; ((fp > 0) && (NO_PREV_FP != fp)); sp = fp, fp = NEXT_FP ())
    {
      reg_t local = fp + FPS;
      u8_t obj_cnt = (sp - local) / sizeof (Object);
      active_root_insert_frame (stack, local, obj_cnt);

      /* NOTE: The closure captured heap-allocated object should be in
       *        active_root too.
       */

      closure_t closure = *((closure_t *)(stack + local - sizeof (closure_t)));
      if (closure && closure->frame_size)
        {
          for (int i = 0; i < closure->frame_size; i++)
            {
              object_t obj = (&((object_t) (stack + closure->local))[i]);
              active_root_inner_insert (obj->attr.type, obj->value);
            }
        }
    }
}

static void clean_active_root ()
{
  /* NOTE: Don't waste time to clean one by one. */
  _arn.index = 0;
  RB_INIT (&ActiveRootHead);
}

static void collect (size_t *count, obj_list_head_t *head, bool hurt,
                     bool force)
{
  obj_list_t node = NULL;

  /* GC algo:
      1. Skip permanent object.
      2. If it 's in active root, it get aged if it' s gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */

  printf ("collect(), FREE_LIST_PRINT (head) = ");
  FREE_LIST_PRINT (head);

  SLIST_FOREACH (node, head, next)
  {
    if (force)
      {
        node->obj->attr.gc = FREE_OBJ;
      }
    else
      {
        int gc = node->obj->attr.gc;

        if (PERMANENT_OBJ == gc)
          {
            continue;
          }
        else if (exist (node->obj))
          {
            if (GEN_1_OBJ == gc)
              {
                // younger object aged
                node->obj->attr.gc++;
              }
            else if (GEN_2_OBJ == gc && hurt)
              {
                // hurtfully collect
                node->obj->attr.gc = FREE_OBJ;
              }
          }
        else
          {
            // Not alive, release it
            node->obj->attr.gc = FREE_OBJ;
          }
      }

    if (FREE_OBJ == node->obj->attr.gc)
      {
        free_object (node->obj);
        (*count)++;
      }
  }
}

static int get_gc_from_node (otype_t type, void *value)
{
  int gc = 0;

  switch (type)
    {
    case pair:
      {
        gc = ((pair_t)value)->attr.gc;
        break;
      }
    case vector:
      {
        gc = ((vector_t)value)->attr.gc;
        break;
      }
    case list:
      {
        gc = ((list_t)value)->attr.gc;
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        gc = ((closure_t)value)->attr.gc;
        break;
      }
    default:
      {
        PANIC ("Invalid node type %d\n", type);
      }
    }

  return gc;
}

static void set_gc_to_node (otype_t type, void *value, int gc)
{
  switch (type)
    {
    case pair:
      {
        ((pair_t)value)->attr.gc = gc;
        break;
      }
    case vector:
      {
        ((vector_t)value)->attr.gc = gc;
        break;
      }
    case list:
      {
        ((list_t)value)->attr.gc = gc;
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        ((closure_t)value)->attr.gc = gc;
        break;
      }
    default:
      {
        PANIC ("Invalid node type %d\n", type);
      }
    }
}

static void collect_inner (size_t *count, obj_list_head_t *head, otype_t type,
                           bool hurt, bool force)
{
  obj_list_t node = NULL;

  /* GC algo:
      1. Skip permanent object.
      2. If it 's in active root, it get aged if it' s gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */
  printf ("collect_inner(), FREE_LIST_PRINT (head) = ");
  FREE_LIST_PRINT (head);

  SLIST_FOREACH (node, head, next)
  {
    int gc = force ? FREE_OBJ : get_gc_from_node (type, (void *)node->obj);

    if (PERMANENT_OBJ == gc)
      {
        continue;
      }
    else if (exist (node->obj))
      {
        if (GEN_1_OBJ == gc)
          {
            // younger object aged
            gc++;
          }
        else if (GEN_2_OBJ == gc && hurt)
          {
            // hurtfully collect
            gc = FREE_OBJ;
          }
      }
    else
      {
        // Not alive, release it
        gc = FREE_OBJ;
      }

    if (FREE_OBJ == gc)
      {
        free_inner_object (type, (void *)node->obj);
        (*count)++;
      }
    else
      {
        set_gc_to_node (type, (void *)node->obj, gc);
      }
  }
}

static size_t count_me (obj_list_head_t *head)
{
  obj_list_t node = NULL;
  size_t cnt = 0;

  SLIST_FOREACH (node, head, next)
  {
    cnt++;
  }
  return cnt;
}

/* static void sweep () */
/* { */
/*   printf ("obj\n"); */
/*   FREE_OBJECTS (&obj_free_pool); */
/*   printf ("pair\n"); */
/*   FREE_OBJECTS (&pair_free_pool); */
/*   printf ("vector\n"); */
/*   FREE_OBJECTS (&vector_free_pool); */
/*   printf ("list\n"); */
/*   FREE_OBJECTS (&list_free_pool); */
/*   printf ("closure\n"); */
/*   FREE_OBJECTS (&closure_free_pool); */
/* } */

bool gc (const gc_info_t gci)
{
  /* TODO:
   * 1. Obj pool is empty, goto 3
   * 2. Free all unused obj:
   *    a. move from ref_list to free_list (obj pool)
   *    b. if no collectable obj, then goto 3
   * 3. Free obj pool
   */
  usleep (10000);

#ifdef LAMBDACHIP_LINUX
  const long long TICKS_PER_SECOND = 1000000L;
  struct timeval tv;
  struct timezone tz;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t cycles_spent;
  uint64_t nanoseconds_spent;
#endif

#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t0 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t0 = k_cycle_get_32 ();
#endif

  build_active_root (gci);

#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t1 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t1 = k_cycle_get_32 ();
#endif

  size_t count = 0;
  size_t delta = 0;

  gc_final = 0;

  printf ("pair before: %d\n", count_me (&pair_free_pool));
  printf ("vector before: %d\n", count_me (&vector_free_pool));
  printf ("list before: %d\n", count_me (&list_free_pool));
  printf ("closure before: %d\n", count_me (&closure_free_pool));
  printf ("obj before: %d\n", count_me (&obj_free_pool));
  /* printf ("obj\n"); */
  /* collect (&count, &obj_free_pool, false); */
  printf ("inner\n");
  collect_inner (&delta, &pair_free_pool, vector, false, false);
  printf ("vector done, count: %d, remain: %d\n", delta,
          count_me (&pair_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &vector_free_pool, closure_on_heap, false, false);
  printf ("obj done, count: %d, remain: %d\n", delta,
          count_me (&vector_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &list_free_pool, list, false, false);
  printf ("list done, count: %d, remain: %d\n", delta,
          count_me (&list_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &closure_free_pool, vector, false, false);
  printf ("closure done, count: %d, remain: %d\n", delta,
          count_me (&closure_free_pool));
  count += delta;
  delta = 0;

#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t2 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t2 = k_cycle_get_32 ();
#endif

  gc_final = 1;

  collect_inner (&delta, &list_free_pool, list, false, false);
  printf ("list done, count: %d, remain: %d\n", delta,
          count_me (&list_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &pair_free_pool, pair, false, false);
  printf ("pair done, count: %d, remain: %d\n", delta,
          count_me (&pair_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &closure_free_pool, closure_on_heap, false, false);
  printf ("closure done, count: %d, remain: %d\n", delta,
          count_me (&closure_free_pool));
  count += delta;
  delta = 0;

  collect_inner (&delta, &vector_free_pool, vector, false, false);
  printf ("vector done, count: %d, remain: %d\n", delta,
          count_me (&vector_free_pool));
  count += delta;
  delta = 0;

  collect (&delta, &obj_free_pool, false, false);
  printf ("obj done, count: %d, remain: %d\n", delta,
          count_me (&obj_free_pool));
  count += delta;
  delta = 0;

  if (0 == count && gci->hurt)
    {
      /*
        NOTE: No memory and no freed object, hurtly collect to release all
              active gen-2 object.

        FIXME: Hurt collect will cause the active node collected intendedly,
               however, this is the edge case if there's no memory to alloc.
               Do we have better approach to avoid big hurt?
               Or do we really need hurt collect in embedded system?
      */
      gc_final = 0;

      collect_inner (&count, &pair_free_pool, pair, true, false);
      collect_inner (&count, &vector_free_pool, vector, true, false);
      collect_inner (&count, &list_free_pool, list, true, false);
      collect_inner (&count, &closure_free_pool, closure_on_heap, true, false);

      gc_final = 1;

      collect_inner (&count, &pair_free_pool, pair, true, false);
      collect_inner (&count, &vector_free_pool, vector, true, false);
      collect_inner (&count, &list_free_pool, list, true, false);
      collect_inner (&count, &closure_free_pool, closure_on_heap, true, false);
      collect (&count, &obj_free_pool, true, false);

      gc_final = 0;
    }

#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t3 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t3 = k_cycle_get_32 ();
#endif

#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t4 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t4 = k_cycle_get_32 ();
#endif
  clean_active_root ();
#ifdef LAMBDACHIP_LINUX
  gettimeofday (&tv, &tz);
  long long t5 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#elif defined(LAMBDACHIP_ZEPHYR)
  uint32_t t5 = k_cycle_get_32 ();
#endif

#ifdef LAMBDACHIP_LINUX
  printf ("%lld, %lld, %lld, %lld, %lld\n", t1 - t0, t2 - t0, t3 - t0, t4 - t0,
          t5 - t0);
#elif defined(LAMBDACHIP_ZEPHYR)
  printf ("%d, %d, %d, %d, %d\n", t1 - t0, t2 - t0, t3 - t0, t4 - t0, t5 - t0);
#endif

  printf ("oln: %d, arn: %d\n", _oln.index, _arn.index);

  os_printk ("pair_free_pool");
  FREE_LIST_PRINT (&pair_free_pool);
  os_printk ("vector_free_pool");
  FREE_LIST_PRINT (&vector_free_pool);
  os_printk ("list_free_pool");
  FREE_LIST_PRINT (&list_free_pool);
  os_printk ("closure_free_pool");
  FREE_LIST_PRINT (&closure_free_pool);
  os_printk ("obj_free_pool");
  FREE_LIST_PRINT (&obj_free_pool);

  return true;
}

void gc_clean_cache (void)
{
  size_t cnt = 0;
  os_printk ("pair_free_pool\n");
  collect_inner (&cnt, &pair_free_pool, pair, false, true);
  os_printk ("vector_free_pool\n");
  collect_inner (&cnt, &vector_free_pool, vector, false, true);
  os_printk ("list_free_pool\n");
  collect_inner (&cnt, &list_free_pool, list, false, true);
  os_printk ("closure_free_pool\n");
  collect_inner (&cnt, &closure_free_pool, closure_on_heap, false, true);

  gc_final = 1;
  os_printk ("gc_final = 1 && collect\n");

  os_printk ("pair_free_pool\n");
  collect_inner (&cnt, &pair_free_pool, pair, false, true);
  os_printk ("vector_free_pool\n");
  collect_inner (&cnt, &vector_free_pool, vector, false, true);
  os_printk ("list_free_pool\n");
  collect_inner (&cnt, &list_free_pool, list, false, true);
  os_printk ("closure_free_pool\n");
  collect_inner (&cnt, &closure_free_pool, closure_on_heap, false, true);
  os_printk ("obj_free_pool\n");
  collect (&cnt, &obj_free_pool, false, true);

  os_printk ("after collect && clean cache\n");
  os_printk ("pair_free_pool\n");
  FREE_LIST_PRINT (&pair_free_pool);
  os_printk ("vector_free_pool\n");
  FREE_LIST_PRINT (&vector_free_pool);
  os_printk ("list_free_pool\n");
  FREE_LIST_PRINT (&list_free_pool);
  os_printk ("closure_free_pool\n");
  FREE_LIST_PRINT (&closure_free_pool);
  os_printk ("obj_free_pool\n");
  FREE_LIST_PRINT (&obj_free_pool);
}

void gc_obj_book (void *obj)
{
  obj_list_t node = NULL;

  node = object_list_node_alloc ();
  if (!node)
    {
      PANIC ("gc_book 0: We're doomed! There're even no RAMs for GC!\n");
    }
  node->obj = obj;
  SLIST_INSERT_HEAD (&obj_free_pool, node, next);
}

void gc_inner_obj_book (otype_t t, void *obj)
{
  obj_list_t node = NULL;
  node = object_list_node_alloc ();
  if (!node)
    {
      PANIC ("gc_book 0: We're doomed! There're even no RAMs for GC!\n");
    }
  node->obj = obj;

  switch (t)
    {
    case pair:
      {
        SLIST_INSERT_HEAD (&pair_free_pool, node, next);
        break;
      }
    case vector:
      {
        SLIST_INSERT_HEAD (&vector_free_pool, node, next);
        break;
      }
    case list:
      {
        SLIST_INSERT_HEAD (&list_free_pool, node, next);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        SLIST_INSERT_HEAD (&closure_free_pool, node, next);
        break;
      }
    default:
      {
        PANIC ("Invalid object type %d", t);
      }
    }
}

void *gc_pool_malloc (otype_t type)
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
  obj_list_t node = NULL;

  switch (type)
    {
    case imm_int:
    case character:
    case real:
    case rational_pos:
    case rational_neg:
    case boolean:
    case null_obj:
    case none:
    case string:
    case symbol:
    case primitive:
    case procedure:
      {
        node = get_free_obj_node (&obj_free_pool);
        break;
      }
    case list:
      {
        node = get_free_obj_node (&list_free_pool);
        break;
      }
    case pair:
      {
        node = get_free_obj_node (&pair_free_pool);
        break;
      }
    case vector:
      {
        node = get_free_obj_node (&vector_free_pool);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        PANIC ("BUG: closures are not allocated from pool!\n");
        break;
      }
    default:
      {
        PANIC ("Invalid object type: %d", type);
      }
    }

  return node ? node->obj : NULL;
}

void simple_collect (obj_list_head_t *head)
{
  obj_list_t node = NULL;

  SLIST_FOREACH (node, head, next)
  {
    object_t obj = (object_t)node->obj;

    if (PERMANENT_OBJ != obj->attr.gc)
      {
        obj->attr.gc = FREE_OBJ;
      }
  }
}

void gc_try_to_recycle (void)
{
  /* FIXME: The runtime created globals shouldn't be recycled */
  simple_collect (&obj_free_pool);
  simple_collect (&list_free_pool);
  simple_collect (&vector_free_pool);
  simple_collect (&pair_free_pool);

  /* NOTE:
   * Closures are not fixed size object, so we have to free it.
   */
  size_t cnt = 0;
  collect_inner (&cnt, &closure_free_pool, closure_on_heap, false, true);
}

void gc_recycle_current_frame (const u8_t *stack, u32_t local, u32_t sp)
{
#if defined GC_RECYCLE_CURRENT_FRAME == 1
  size_t size = sizeof (Object);
  size_t cnt = (sp - local) / size;

  for (size_t i = 0; i < cnt; i++)
    {
      object_t obj = (object_t) (stack + local + i * size);

      switch (obj->attr.type)
        {
        case imm_int:
        case character:
        case real:
        case rational_pos:
        case rational_neg:
        case boolean:
        case null_obj:
        case none:
        case string:
        case symbol:
        case primitive:
        case procedure:
          {
            // non-heap object
            // printf ("non heap obj: %d\n", obj->attr.type);
            break;
          }
        case closure_on_heap:
        case closure_on_stack:
          {
            // closures are never recycled, we just free them
            obj->attr.gc = FREE_OBJ;
            free_object_from_pool (&closure_free_pool, obj);
            break;
          }
        default:
          {
            recycle_object (obj);
          }
        }
    }
#endif
}

void gc_init (void)
{
  pre_allocate_active_nodes ();
  object_list_node_pre_allocate ();

  SLIST_INIT (&obj_free_pool);
  SLIST_INIT (&list_free_pool);
  SLIST_INIT (&vector_free_pool);
  SLIST_INIT (&pair_free_pool);
  SLIST_INIT (&closure_free_pool);
}

void gc_clean (void)
{
  active_nodes_clean ();
  object_list_node_clean ();
}

// remove first find object in LIST head
static void free_object_from_pool (obj_list_head_t *head, object_t o)
{
  if (!gc_final)
    {
      return;
    }

  obj_list_t node = NULL;
  SLIST_FOREACH (node, (head), next)
  {
    if (node->obj == (o))
      {
        os_free (node->obj);
        node->obj = (void *)0xBEAFDEAD;
        // node->obj = NULL;
        SLIST_REMOVE (head, node, ObjectList, next);
        object_list_node_recycle (node);
        break;
      }
  }
}
