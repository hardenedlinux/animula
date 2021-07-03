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

/* The GC in LambdaChip is "object-based generational GC".
   We don't perform mark/sweep, or any reference counting.

   The meaning of `gc' field in Object:
   * 3 means permernant.
   * 1~2 means the generation, 0 means free.
   * The `gc' will increase by 1 when it survives from GC.
   * For stack-allocated object, `gc' field is always 0.

 */

static RB_HEAD (ActiveRoot, ActiveRootNode)
  ActiveRootHead = RB_INITIALIZER (&ActiveRootHead);

RB_GENERATE_STATIC (ActiveRoot, ActiveRootNode, entry, active_root_compare);

static obj_list_head_t obj_free_list;
static obj_list_head_t oln_free_list;
static obj_list_head_t list_free_list;
static obj_list_head_t vector_free_list;
static obj_list_head_t pair_free_list;
static obj_list_head_t closure_free_list;
static obj_list_head_t procedure_free_list;

static struct Pre_ARN _arn = {0};
static struct Pre_OLN _oln = {0};

static void pre_allocate_active_nodes (void)
{
  for (int i = 0; i < PRE_ARN; i++)
    {
      _arn.arn[i] = (ActiveRootNode *)os_malloc (sizeof (ActiveRootNode));

      if (NULL == _arn.arn[i])
        {
          os_printk ("GC: We're doomed! Did you set a too large PRE_ARN?");
          panic ("Try to set PRE_ARN smaller!");
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

static void pre_allocate_obj_list_nodes (void)
{
  for (int i = 0; i < PRE_OLN; i++)
    {
      _oln.oln[i] = (obj_list_t)os_malloc (sizeof (ObjectList));

      if (NULL == _oln.oln[i])
        {
          PANIC ("GC: We're doomed! Did you set a too large PRE_OLN?"
                 "Try to set PRE_OLN smaller!");
        }
      _oln.cnt++;
    }

  VM_DEBUG ("PRE_OLN: %d, cnt: %d, pre-allocate %d bytes.\n", PRE_OLN, _oln.cnt,
            PRE_OLN * sizeof (ObjectList));
}

obj_list_t oln_alloc (void)
{
  obj_list_t ret = NULL;

  if (0 == _oln.cnt)
    {
      return NULL;
    }

  for (int i = 0; i < PRE_OLN; i++)
    {
      ret = _oln.oln[i];

      if (NULL != ret)
        {
          _oln.oln[i] = NULL;
          _oln.cnt--;
          break;
        }
    }

  if (NULL == ret)
    {
      os_printk ("BUG: there's no obj_list node, but cnt is %d\n", _oln.cnt);
      panic ("Maybe it's not recycled correctly?");
    }

  return ret;
}

static void obj_list_node_recycle (obj_list_t node)
{
  for (int i = 0; i < PRE_OLN; i++)
    {
      if (NULL == _oln.oln[i])
        {
          _oln.oln[i] = node;
          _oln.cnt++;
          break;
        }
    }
}

bool is_oln_available (void)
{
  return (0 != _oln.cnt);
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

static void obj_list_nodes_clean (void)
{
  for (int i = 0; i < PRE_OLN; i++)
    {
      os_free (_oln.oln[i]);
      _oln.cnt--;
    }

  if (0 != _oln.cnt)
    {
      os_printk (
        "obj_list_nodes_clean: "
        "the cnt (%d) is not 0, maybe there's potential bug somewhere!\n",
        _oln.cnt);
      panic ("");
    }
  VM_DEBUG ("OLN clean!\n");
}

static void free_object (object_t obj)
{
  /* NOTE: Integers are self-contained object, so we can just release the object
   */
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
        FREE_OBJECT (&pair_free_list, (object_t)obj->value);
        break;
      }
    case list:
      {
        obj_list_t node = NULL;
        obj_list_t prev = NULL;
        obj_list_head_t *head = &((list_t)obj->value)->list;

        SLIST_FOREACH (node, head, next)
        {
          if (prev)
            {
              SLIST_REMOVE (head, prev, ObjectList, next);
              free_object ((object_t)prev->obj);
              prev->obj = NULL;
              obj_list_node_recycle (prev);
            }
          prev = node;
        }

        obj_list_node_recycle (prev); // free the last node
        FREE_OBJECT (&list_free_list, (object_t)obj->value);
        break;
      }
    case continuation:
    case mut_string:
      {
        os_free ((void *)obj->value);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        FREE_OBJECT (&closure_free_list, (object_t)obj);
        break;
      }
    default:
      {
        PANIC ("free_object: Invalid type %d!\n", obj->attr.type);
      }
    }

  if (0 != obj->attr.gc)
    {
      FREE_OBJECT (&obj_free_list, obj);
    }
}

static void recycle_object (object_t obj)
{
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
      {
        RECYCLE_OBJ (obj_free_list);
        break;
      }
    case list:
      {
        RECYCLE_OBJ (list_free_list);
        break;
      }
    case pair:
      {
        RECYCLE_OBJ (pair_free_list);
        break;
      }
    case vector:
      {
        RECYCLE_OBJ (vector_free_list);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        // closures don't need to recycle
        FORCE_FREE_OBJECTS (&closure_free_list);
        break;
      }
    case procedure:
      {
        RECYCLE_OBJ (procedure_free_list);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", obj->attr.type);
        panic ("recycle_object is down!");
      }
    }
}

static inline void insert (ActiveRootNode *an)
{
  RB_INSERT (ActiveRoot, &ActiveRootHead, an);
}

static inline bool exist (object_t obj)
{

  ActiveRootNode node = {.value = (void *)obj};
  return NULL != RB_FIND (ActiveRoot, &ActiveRootHead, &node);
}

static void active_root_insert (object_t obj)
{
  if (exist (obj))
    return;

  switch (obj->attr.type)
    {
    case imm_int:
    case primitive:
    case string:
      {
        // Self-contain object
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
        obj_list_head_t *head = &((list_t)obj->value)->list;

        SLIST_FOREACH (node, head, next)
        {
          active_root_insert (node->obj);
        }
        break;
      }
    case vector:
      {
        panic ("GC: Hey, did we support Vector now? If so, please fix me!\n");
        break;
      }
    default:
      {
        /* procedure */
        /* closure_on_heap */
        /* closure_on_stack */
        /* mut_string */
        break;
      }
    }

  ActiveRootNode *an = arn_alloc ();
  an->value = (void *)obj;
  insert (an);
}

static void active_root_insert_frame (u8_t *stack, u32_t local, u8_t cnt)
{
  for (u8_t i = 0; i < cnt; i++)
    {
      object_t obj = (object_t) (stack + local);

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
  u32_t fp = gci->fp;
  u32_t sp = gci->sp;

  for (; fp; sp = fp, fp = NEXT_FP ())
    {
      u32_t local = fp + FPS;
      u8_t obj_cnt = (sp - local) / sizeof (Object);
      active_root_insert_frame (stack, local, obj_cnt);
    }

  /* FIXME: The closure captured heap-allocated object should be in
   *        active_root too.
   */
}

static void clean_active_root ()
{
  _arn.index = 0;
  RB_HEAD (ActiveRoot, ActiveRootNode)
  ActiveRootHead = RB_INITIALIZER (&ActiveRootHead);
}

static void collect (size_t *count, obj_list_head_t *head, bool hurt)
{
  obj_list_t node = NULL;

  /* GC algo:
      1. Skip permanent object.
      2. If it 's in active root, it get aged if it' s gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */

  SLIST_FOREACH (node, head, next)
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
            // hurtly collect
            node->obj->attr.gc = FREE_OBJ;
          }
      }
    else
      {
        // Not alive, release it
        node->obj->attr.gc = FREE_OBJ;
      }

    if (FREE_OBJ == node->obj->attr.gc)
      {
        recycle_object (node->obj);
        *count++;
      }
  }
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

  size_t count = 0;
try_collect:
  collect (&count, &obj_free_list, gci->hurt);
  collect (&count, &list_free_list, gci->hurt);
  collect (&count, &vector_free_list, gci->hurt);
  collect (&count, &pair_free_list, gci->hurt);
  collect (&count, &closure_free_list, gci->hurt);
  collect (&count, &procedure_free_list, gci->hurt);

  if (0 == count && false == gci->hurt)
    {
      /*
        NOTE: No memory and no freed object, hurtly collect to release all
              active gen-2 object.

        FIXME: Hurt collect will cause the active node collected intendedly,
               however, this is the edge case if there's no memory to alloc.
               Do we have better approach to avoid big hurt?
               Or do we really need hurt collect in embedded system?
      */
      gci->hurt = true;
      goto try_collect;
    }

  clean_active_root ();
  return false;
}

// For list node container
static void oln_free_list_clean (void)
{
  obj_list_t prev = NULL;
  obj_list_t node = NULL;

  SLIST_FOREACH (node, &oln_free_list, next)
  {
    if (prev)
      {
        SLIST_REMOVE (&oln_free_list, prev, ObjectList, next);
        os_free (prev);
      }
    prev = node;
  }

  os_free (prev);
}

void gc_clean_cache (void)
{
  FORCE_FREE_OBJECTS (&obj_free_list);
  FORCE_FREE_OBJECTS (&list_free_list);
  FORCE_FREE_OBJECTS (&vector_free_list);
  FORCE_FREE_OBJECTS (&pair_free_list);
  FORCE_FREE_OBJECTS (&closure_free_list);
  FORCE_FREE_OBJECTS (&procedure_free_list);
  oln_free_list_clean ();
}

void gc_oln_book (obj_list_t node)
{
  SLIST_INSERT_HEAD (&oln_free_list, node, next);
}

void gc_book (otype_t type, object_t obj, obj_list_t node)
{
  if (!node)
    node = oln_alloc ();

  if (!node)
    {
      panic ("gc_book: We're doomed! There're even no RAMs for GC!\n");
    }

  node->obj = obj;

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
      {
        SLIST_INSERT_HEAD (&obj_free_list, node, next);
        break;
      }
    case list:
      {
        SLIST_INSERT_HEAD (&list_free_list, node, next);
        break;
      }
    case pair:
      {
        SLIST_INSERT_HEAD (&pair_free_list, node, next);
        break;
      }
    case vector:
      {
        SLIST_INSERT_HEAD (&vector_free_list, node, next);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        node->obj = obj->value;
        SLIST_INSERT_HEAD (&closure_free_list, node, next);
        break;
      }
    case procedure:
      {
        SLIST_INSERT_HEAD (&procedure_free_list, node, next);
        break;
      }
    default:
      {
        os_printk ("Invalid object type %d\n", obj->attr.type);
        panic ("gc_book is down!");
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
  void *ret = NULL;

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
      {
        MALLOC_OBJ_FROM_POOL (obj_free_list);
        break;
      }
    case obj_list_node:
      {
        // Don't cache oln
        break;
      }
    case list:
      {
        MALLOC_OBJ_FROM_POOL (list_free_list);
        break;
      }
    case pair:
      {
        MALLOC_OBJ_FROM_POOL (pair_free_list);
        break;
      }
    case vector:
      {
        MALLOC_OBJ_FROM_POOL (vector_free_list);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        panic ("BUG: closures are not allocated from pool!\n");
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

void simple_collect (obj_list_head_t *head)
{
  obj_list_t node = NULL;

  SLIST_FOREACH (node, head, next)
  {
    object_t obj = (object_t)node->obj;

    if (PERMANENT_OBJ != obj->attr.gc)
      {
        obj->attr.gc = 0;
      }
  }
}

void gc_try_to_recycle (void)
{
  /* FIXME: The runtime created globals shouldn't be recycled */
  simple_collect (&obj_free_list);
  simple_collect (&list_free_list);
  simple_collect (&vector_free_list);
  simple_collect (&pair_free_list);
  simple_collect (&procedure_free_list);

  /* NOTE:
   * Closures are not fixed size object, so we have to free it.
   */
  FORCE_FREE_OBJECTS (&closure_free_list);
  oln_free_list_clean ();
}

void gc_init (void)
{
  pre_allocate_active_nodes ();
  pre_allocate_obj_list_nodes ();

  SLIST_INIT (&obj_free_list);
  SLIST_INIT (&oln_free_list);
  SLIST_INIT (&list_free_list);
  SLIST_INIT (&vector_free_list);
  SLIST_INIT (&pair_free_list);
  SLIST_INIT (&closure_free_list);
  SLIST_INIT (&procedure_free_list);
}

void gc_clean (void)
{
  active_nodes_clean ();
  obj_list_nodes_clean ();
}
