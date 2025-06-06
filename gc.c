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

#ifdef USE_OBG_GC

#  include "list.h"
#  include "obg_gc.h"

#  ifdef ANIMULA_LINUX
#    include <sys/time.h>

#  endif

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

static ListHead pair_free_pool;
static ListHead vector_free_pool;
static ListHead list_free_pool;
static ListHead closure_free_pool;
static ListHead bytevector_free_pool;
static ListHead mut_bytevector_free_pool;
static ListHead obj_free_pool;

static struct Pre_ARN _arn = {0};
// TODO: static
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
  return _arn.arn[_arn.index++];
}

static void object_list_node_pre_allocate (void)
{
  int i = 0;
  for (; i < PRE_OLN; i++)
    {
      list_node_t ptr = (list_node_t)os_malloc (sizeof (ListNode));
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
            PRE_OLN * sizeof (ListNode));
}

list_node_t object_list_node_alloc (void)
{
  list_node_t ret = NULL;

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

// put list_node_t back into OLN for future use
static void object_list_node_recycle (list_node_t node)
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
  VM_DEBUG ("ARN clean!\n");
}

static void object_list_node_clean (void)
{
  // do not modify i to start from 0, which will cost you at least $2000 USD
  if (0 != _oln.index)
    {
      PANIC ("Not all nodes returned to OLN");
    }
  for (int i = _oln.index; i < PRE_OLN; i++)
    {
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

void free_object (object_t obj)
{
  if (0xDEADBEEF == (uintptr_t)obj)
    {
      os_printk ("active_root_insert: oh a half list node!\n");
      os_printk ("let's skip it safely!\n");
      return;
    }

  if (!obj)
    {
      PANIC ("BUG: free a null object!");
    }

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
        // simple object, we don't need to free its value
        // no need to free string
        // symbol should never be recycled
        break;
      }
    case pair:
      {
        free_object ((object_t) ((pair_t)obj->value)->car);
        free_object ((object_t) ((pair_t)obj->value)->cdr);
        break;
      }
    case list:
      {
        list_node_t node = NULL;
        ListHead *head = LIST_OBJECT_HEAD (obj);
        u16_t non_shared = LIST_OBJECT_SIDX (obj);
        u16_t cnt = 0;

        if (!SLIST_EMPTY (head))
          {
            node = SLIST_FIRST (head);
            while (node)
              {
                // call free_object recursively since node->obj can be a
                // composite object
                if (cnt == non_shared)
                  {
                    // Skip the shared partition
                    break;
                  }

                free_object (node->obj);
                SLIST_REMOVE_HEAD (head, next);
                os_free (node);
                // instead of free node, put node into OLN for future use
                node = SLIST_FIRST (head);
                cnt++;
              }
          }
        break;
      }
    case continuation:
    case mut_string:
    case closure_on_heap:
    case closure_on_stack:
    case bytevector:
      {
        os_free ((void *)obj->value);
        break;
      }
    case mut_bytevector:
      {
        os_free (((mut_bytevector_t) (obj->value))->vec);
        os_free (obj->value);
        break;
      }
    default:
      {
        PANIC ("free_object: Invalid type %d!\n", obj->attr.type);
      }
    }

  obj->attr.gc = FREE_OBJ;
}

void free_inner_object (otype_t type, void *value)
{
  /* NOTE: Integers are self-contained object, so we can just release the
   * object
   */
  if (!value)
    {
      PANIC ("BUG: free a null object!");
    }

  u8_t gc = get_gc_from_node (type, value);

  if (PERMANENT_OBJ == gc)
    return;

  switch (type)
    {
    case pair:
      {
        free_object ((object_t) ((pair_t)value)->car);
        free_object ((object_t) ((pair_t)value)->cdr);
        ((pair_t)value)->attr.gc = FREE_OBJ;
        break;
      }
    case list:
      {
        list_node_t node = NULL;
        ListHead *head = &((list_t)value)->list;
        u16_t non_shared = ((list_t)value)->non_shared;
        u16_t cnt = 0;

        if (!SLIST_EMPTY (head))
          {
            node = SLIST_FIRST (head);
            while (node)
              {
                // call free_object recursively since node->obj can be a
                // composite object
                if (cnt == non_shared)
                  {
                    break;
                  }
                free_object (node->obj);
                SLIST_REMOVE_HEAD (head, next);
                os_free (node);
                // instead of free node, put node into OLN for future use
                node = SLIST_FIRST (head);
                cnt++;
              }
          }
        ((list_t)value)->attr.gc = FREE_OBJ;
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        ((closure_t)value)->attr.gc = FREE_OBJ;
        break;
      }
    case bytevector:
      {
        ((bytevector_t)value)->attr.gc = FREE_OBJ;
        break;
      }
    case mut_bytevector:
      {
        ((mut_bytevector_t)value)->attr.gc = FREE_OBJ;
        os_free (((mut_bytevector_t)value)->vec);
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
        list_node_t node = NULL;
        ListHead *head = LIST_OBJECT_HEAD (obj);

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
    case bytevector:
      {
        free_object_from_pool (&bytevector_free_pool, obj);
        break;
      }
    case mut_bytevector:
      {
        free_object_from_pool (&mut_bytevector_free_pool, obj);
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
        list_node_t node = NULL;
        ListHead *head = &((list_t)obj->value)->list;

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
        list_node_t node = NULL;
        ListHead *head = &((list_t)value)->list;

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

static void collect (size_t *count, ListHead *head, bool hurt, bool force)
{
  list_node_t node = NULL;

  /* GC algo:
      1. Skip permanent object.
      2. If it 's in active root, it get aged if it' s gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */
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
                node->obj->attr.gc = GEN_2_OBJ;
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
    case bytevector:
      {
        gc = ((bytevector_t)value)->attr.gc;
        break;
      }
    case mut_bytevector:
      {
        gc = ((mut_bytevector_t)value)->attr.gc;
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

static void collect_inner (size_t *count, ListHead *head, otype_t type,
                           bool hurt, bool force)
{
  list_node_t node = NULL;

  /* GC algo:
      1. Skip permanent object.
      2. If it's in active root, it get aged if it's gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */
  SLIST_FOREACH (node, head, next)
  {
    u8_t gc = force ? FREE_OBJ : get_gc_from_node (type, (void *)node->obj);

    if (PERMANENT_OBJ == gc)
      {
        continue;
      }
    else if (exist (node->obj))
      {
        if (GEN_1_OBJ == gc)
          {
            // younger object aged
            gc = GEN_2_OBJ;
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

static size_t count_me (ListHead *head)
{
  list_node_t node = NULL;
  size_t cnt = 0;

  SLIST_FOREACH (node, head, next)
  {
    cnt++;
  }
  return cnt;
}

static void release_all_free_objects (ListHead *head, bool force)
{
  list_node_t node = NULL;
  list_node_t nxt = NULL;

  if (!SLIST_EMPTY (head))
    {
      node = SLIST_FIRST (head);
      while (node)
        {
          // call free_object recursively since node->obj can be a
          // composite object
          if ((FREE_OBJ == node->obj->attr.gc) || force)
            {
              /* printf ("release node: %p, obj: %p, value: %p\n", node,
               * node->obj, */
              /*         node->obj->value); */
              os_free (node->obj);
              // instead of free node, put node into OLN for future use
              SLIST_REMOVE (head, node, ListNode, next);
              nxt = SLIST_NEXT (node, next);
              object_list_node_recycle (node);
              node = nxt;
            }
          else
            {
              node = SLIST_NEXT (node, next);
            }
        }
    }
}

static void sweep (bool force)

{
  VM_DEBUG ("sweep pair\n");
  release_all_free_objects (&pair_free_pool, force);
  VM_DEBUG ("sweep vector\n");
  release_all_free_objects (&vector_free_pool, force);
  VM_DEBUG ("sweep list\n");
  release_all_free_objects (&list_free_pool, force);
  VM_DEBUG ("sweep closure\n");
  release_all_free_objects (&closure_free_pool, force);
  VM_DEBUG ("sweep bytevector\n");
  release_all_free_objects (&bytevector_free_pool, force);
  VM_DEBUG ("sweep mut_bytevector\n");
  release_all_free_objects (&mut_bytevector_free_pool, force);
  VM_DEBUG ("sweep obj\n");
  release_all_free_objects (&obj_free_pool, force);
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
  // usleep (10000);

#  ifdef ANIMULA_LINUX
  const long long TICKS_PER_SECOND = 1000000L;
  struct timeval tv;
  struct timezone tz;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t cycles_spent;
  uint64_t nanoseconds_spent;
#  endif

#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t0 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t0 = k_cycle_get_32 ();
#  endif

  build_active_root (gci);

#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t1 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t1 = k_cycle_get_32 ();
#  endif

  size_t count = 0;

  collect_inner (&count, &pair_free_pool, pair, false, false);
  collect_inner (&count, &vector_free_pool, vector, false, false);
  collect_inner (&count, &list_free_pool, list, false, false);
  collect_inner (&count, &closure_free_pool, closure_on_heap, false, false);
  collect_inner (&count, &bytevector_free_pool, bytevector, false, false);
  collect_inner (&count, &mut_bytevector_free_pool, mut_bytevector, false,
                 false);
  collect (&count, &obj_free_pool, false, false);

#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t2 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t2 = k_cycle_get_32 ();
#  endif

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
      collect_inner (&count, &pair_free_pool, pair, true, false);
      collect_inner (&count, &vector_free_pool, vector, true, false);
      collect_inner (&count, &list_free_pool, list, true, false);
      collect_inner (&count, &closure_free_pool, closure_on_heap, true, false);
      collect_inner (&count, &bytevector_free_pool, bytevector, true, false);
      collect_inner (&count, &mut_bytevector_free_pool, mut_bytevector, true,
                     false);
      collect (&count, &obj_free_pool, false, false);
    }

  sweep (false);

#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t3 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t3 = k_cycle_get_32 ();
#  endif

#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t4 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t4 = k_cycle_get_32 ();
#  endif
  clean_active_root ();
#  ifdef ANIMULA_LINUX
  gettimeofday (&tv, &tz);
  long long t5 = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec;
#  elif defined(ANIMULA_ZEPHYR)
  uint32_t t5 = k_cycle_get_32 ();
#  endif

#  ifdef ANIMULA_LINUX
  VM_DEBUG ("%lld, %lld, %lld, %lld, %lld\n", t1 - t0, t2 - t0, t3 - t0,
            t4 - t0, t5 - t0);
#  elif defined(ANIMULA_ZEPHYR)
  VM_DEBUG ("%d, %d, %d, %d, %d\n", t1 - t0, t2 - t0, t3 - t0, t4 - t0,
            t5 - t0);
#  endif

  return true;
}

void gc_clean_cache (void)
{
  size_t cnt = 0;
  collect_inner (&cnt, &pair_free_pool, pair, false, true);
  collect_inner (&cnt, &vector_free_pool, vector, false, true);
  collect_inner (&cnt, &list_free_pool, list, false, true);
  collect_inner (&cnt, &closure_free_pool, closure_on_heap, false, true);
  collect_inner (&cnt, &bytevector_free_pool, bytevector, false, true);
  collect_inner (&cnt, &mut_bytevector_free_pool, mut_bytevector, false, true);

  // free self-contained object in sweep
  sweep (true);
}

void gc_obj_book (void *obj)
{
  list_node_t node = NULL;

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
  list_node_t node = NULL;
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
    case bytevector:
      {
        SLIST_INSERT_HEAD (&bytevector_free_pool, node, next);
        break;
      }
    case mut_bytevector:
      {
        SLIST_INSERT_HEAD (&mut_bytevector_free_pool, node, next);
        break;
      }
    default:
      {
        PANIC ("Invalid object type %d\n", t);
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
  list_node_t node = NULL;

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
    case bytevector:
      {
        node = get_free_obj_node (&bytevector_free_pool);
        break;
      }
    case mut_bytevector:
      {
        node = get_free_obj_node (&mut_bytevector_free_pool);
        break;
      }
    default:
      {
        PANIC ("Invalid object type: %d", type);
      }
    }

  return node ? node->obj : NULL;
}

void simple_collect (ListHead *head)
{
  list_node_t node = NULL;

  SLIST_FOREACH (node, head, next)
  {
    object_t obj = (object_t)node->obj;

    if (PERMANENT_OBJ != obj->attr.gc)
      {
        obj->attr.gc = FREE_OBJ;
      }
  }
}

// collect all composite object, including vector, list, pair
// bytevector is not included, since it does not have child objects with
// attr.gc
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
  release_all_free_objects (&closure_free_pool, true);
}

void gc_recycle_current_frame (const u8_t *stack, u32_t local, u32_t sp)
{
#  if defined GC_RECYCLE_CURRENT_FRAME == 1
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
        case pair:
        case list:
        case mut_list:
        case vector:
        case bytevector:
        case mut_bytevector:
        case mut_string:
        case keyword:
        case continuation:
          {
            recycle_object (obj);
            break;
          }
        case complex_exact:
        case complex_inexact:
        default:
          {
            // defensive programming
            PANIC ("Type not implemented, type: %d", obj->attr.type);
          }
        }
    }
#  endif
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
  SLIST_INIT (&bytevector_free_pool);
  SLIST_INIT (&mut_bytevector_free_pool);
}

void gc_clean (void)
{
  active_nodes_clean ();
  object_list_node_clean ();
}

// remove first find object in LIST head
static void free_object_from_pool (ListHead *head, object_t o)
{
  list_node_t node = NULL;
  SLIST_FOREACH (node, (head), next)
  {
    if (node->obj == (o))
      {
        os_free (node->obj);
        node->obj = NULL;
        SLIST_REMOVE (head, node, ListNode, next);
        object_list_node_recycle (node);
        break;
      }
  }
}
#endif
