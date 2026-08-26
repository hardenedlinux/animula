/*  Copyright (C) 2020-2025
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

// Ensure active_root_compare is visible for RB_GENERATE_STATIC
// It's defined as static inline in obg_gc.h, which is fine

RB_GENERATE_STATIC(ActiveRoot, ActiveRootNode, entry, active_root_compare);

#  ifdef ANIMULA_LINUX
#    include <sys/time.h>

#  endif

static int get_gc_from_node (otype_t type, void *value);
static void set_gc_to_node (otype_t type, void *value, int gc);
static void object_list_node_recycle (list_node_t node);
static void free_object_from_pool (ListHead *head, void *o);
static void free_list_nodes (list_t l, void (*visit) (object_t obj));

/* X-Macro table of every "inner" object kind that lives in its own
 * fixed-size free pool and is registered/collected in a uniform way.
 *
 *   X(enum_tag, c_type, free_pool_variable)
 *
 * Closures are deliberately NOT in this table: closure_on_heap and
 * closure_on_stack are two otype_t tags that share a single pool
 * (closure_free_pool), so the few functions below that need closure
 * handling add it by hand right after the generated cases.
 *
 * This is the answer to "C99 has no generics, so this is repetitive":
 * every place that used to hand-write the same 5-way (or 6-way, with
 * closures) switch now expands from this one list instead.
 */
#  define GC_INNER_TYPE_LIST(X)                                   \
    X (pair, pair_t, pair_free_pool)                               \
    X (vector, vector_t, vector_free_pool)                         \
    X (list, list_t, list_free_pool)                               \
    X (bytevector, bytevector_t, bytevector_free_pool)              \
    X (mut_bytevector, mut_bytevector_t, mut_bytevector_free_pool)

/* Same 5, plus closure (represented by closure_on_heap alone -- fine for
 * call sites that just need "the pool" and "a" type tag for it, unlike
 * get_gc_from_node/set_gc_to_node/gc_inner_obj_book which must list both
 * closure_on_heap and closure_on_stack as separate case labels).
 */
#  define GC_ALL_POOLS_LIST(X) \
    GC_INNER_TYPE_LIST (X)     \
    X (closure_on_heap, closure_t, closure_free_pool)

/* The GC in Animula is "object-based generational GC".
   We don't perform mark/sweep, or any reference counting.

   The meaning of `gc' field in Object:
   * 3 means permarnent.
   * 1~2 means the generation, 0 means free.
   * The `gc' will increase by 1 when it survives from GC.
   * For stack-allocated object, `gc' field is always 0.

 */

// Active root: a red-black tree keyed by pointer value, giving O(log n)
// membership checks instead of the O(n) linear scan this used to be.
static struct ActiveRoot active_root_tree = {NULL};

// See obg_gc.h: set once by the person via gc_bind_vm(vm), right after
// creating/initializing their VM. Deliberately NOT wired up inside
// vm.c -- reaching a global VM state into a shared, backend-agnostic
// file just to serve this one GC backend's internals would be the
// wrong direction of coupling.
vm_t g_current_vm = NULL;

void gc_bind_vm (vm_t vm)
{
  g_current_vm = vm;
}

// Set only by gc_teardown(), for the duration of its one-time final
// pass. free_object/free_inner_object each have their own independent
// "PERMANENT_OBJ objects are never touched" guard -- correct for every
// normal collection, but it silently defeats gc_teardown's whole
// purpose: collect_inner(force=true) bypasses *its own* permanent
// check fine and calls free_inner_object, which then immediately bails
// on *its own*, separate check before ever tearing down internal
// structures (e.g. a list_t's ListNode chain). sweep(true) then
// physically os_frees the outer struct anyway via
// release_all_free_objects's own independent force check (which never
// calls free_inner_object at all), orphaning whatever internal
// structure was never torn down. This flag lets gc_teardown()
// override just those two guards, without touching collect/
// collect_inner's own force semantics or recycle_object's guard
// (recycle_object is never called from gc_teardown, so it's left
// as-is).
static bool g_gc_force_teardown = false;

// Proactive GC trigger. Counts allocation attempts since the last
// collection (of any kind); once GC_ALLOC_THRESHOLD is reached, tells
// the caller to collect and resets. Without this, GC only ever ran
// reactively -- when an allocation had already failed -- which means a
// long-running target that never happens to hit that condition would
// never run a single collection, no matter how much garbage piled up.
//
// Builders may tune this for their target's RAM budget in compiling.
// -D GC_ALLOC_THRESHOLD=1024
#ifndef GC_ALLOC_THRESHOLD
#  define GC_ALLOC_THRESHOLD 256
#endif
static size_t alloc_since_last_gc = 0;

bool gc_alloc_budget_exceeded (void)
{
  if (++alloc_since_last_gc >= GC_ALLOC_THRESHOLD)
    {
      alloc_since_last_gc = 0;
      return true;
    }

  return false;
}

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
  RB_INSERT (ActiveRoot, &active_root_tree, an);
}

static inline bool exist (object_t obj)
{
  ActiveRootNode key = {.value = (void *)obj};
  return NULL != RB_FIND (ActiveRoot, &active_root_tree, &key);
}

static void insert_value (void *value)
{
  ActiveRootNode *an = arn_alloc ();
  an->value = value;
  insert (an);
}

// Free (for free_object/free_inner_object) or recycle (for
// recycle_object) the privately-owned prefix of a list_t's internal
// ListNode chain, calling `visit` on each element's object_t before
// removing and os_free'ing its node.
//
// non_shared is a literal count: exactly this many nodes from the
// head are privately owned by this list_t and safe to free here.
// Anything beyond that is a shared tail borrowed from another list_t's
// own chain -- e.g. `_cdr` (list.c) points a new list_t directly at an
// existing list's second node without allocating anything of its own
// (non_shared=0: nothing here is private, free none of it), or
// `_list_append` builds a fresh private prefix then links its last
// node directly into the second list's existing chain (non_shared =
// length of the fresh prefix). Freeing anything past non_shared here
// would free memory another list_t still owns, causing a
// use-after-free (or double-free) when that other list_t is later
// torn down independently. Every constructor of a fully
// privately-owned list (list literals, map) must set non_shared to
// its own real node count, not 0 -- 0 here specifically means "zero
// private nodes", not "no sharing at all".
static void free_list_nodes (list_t l, void (*visit) (object_t obj))
{
  ListHead *head = &l->list;
  u16_t to_free = l->non_shared;

  if (SLIST_EMPTY (head))
    return;

  list_node_t node = SLIST_FIRST (head);

  for (u16_t i = 0; i < to_free && node; i++)
    {
      // call visit recursively since node->obj can be a composite object
      visit (node->obj);
      list_node_t next_node = SLIST_NEXT (node, next);
      SLIST_REMOVE (head, node, ListNode, next);
      os_free (node);
      node = next_node;
    }
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

  if (PERMANENT_OBJ == obj->attr.gc && !g_gc_force_teardown)
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
        free_object ((object_t)((pair_t)obj->value)->car);
        free_object ((object_t)((pair_t)obj->value)->cdr);
        break;
      }
    case list:
      {
        free_list_nodes ((list_t)obj->value, free_object);
        break;
      }
    case vector:
      {
        vector_t v = (vector_t)obj->value;
        for (u16_t i = 0; i < v->size; i++)
          {
            free_object (v->vec[i]);
          }
        // Tracked in its own pool (vector_free_pool); that pool's own
        // collect_inner + sweep cycle owns the actual os_free of both
        // the Vector struct and its .vec array (see
        // free_inner_object's vector case) -- just mark it dead here.
        set_gc_to_node (obj->attr.type, obj->value, FREE_OBJ);
        break;
      }
    case continuation:
    case mut_string:
      {
        // Not tracked in any inner free_pool (see gc_inner_obj_book),
        // so this Object wrapper is the sole owner of the memory.
        os_free ((void *)obj->value);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
    case bytevector:
    case mut_bytevector:
      {
        // These ARE tracked in their own pool (closure_free_pool /
        // bytevector_free_pool / mut_bytevector_free_pool), whose own
        // collect_inner + sweep cycle owns the actual os_free. Freeing
        // the memory here too would double-free it -- just mark it
        // dead and let that pool take it from here.
        set_gc_to_node (obj->attr.type, obj->value, FREE_OBJ);
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

  if (PERMANENT_OBJ == gc && !g_gc_force_teardown)
    return;

  switch (type)
    {
    case pair:
      {
        free_object ((object_t)((pair_t)value)->car);
        free_object ((object_t)((pair_t)value)->cdr);
        ((pair_t)value)->attr.gc = FREE_OBJ;
        break;
      }
    case list:
      {
        list_t l = (list_t)value;
        free_list_nodes (l, free_object);
        l->attr.gc = FREE_OBJ;
        break;
      }
    case vector:
      {
        // Elements were already recursively torn down by free_object's
        // vector case (called on the outer wrapper before this inner
        // value's own turn comes up) -- here we only own .vec itself.
        vector_t v = (vector_t)value;
        os_free (v->vec);
        v->attr.gc = FREE_OBJ;
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
        free_list_nodes ((list_t)obj->value, recycle_object);
        break;
      }
    case vector:
      {
        vector_t v = (vector_t)obj->value;
        for (u16_t i = 0; i < v->size; i++)
          {
            recycle_object (v->vec[i]);
          }
        free_object_from_pool (&vector_free_pool, obj->value);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        free_object_from_pool (&closure_free_pool, obj->value);
        break;
      }
    case bytevector:
      {
        free_object_from_pool (&bytevector_free_pool, obj->value);
        break;
      }
    case mut_bytevector:
      {
        free_object_from_pool (&mut_bytevector_free_pool, obj->value);
        break;
      }
    case mut_string:
      {
        // Not tracked in any pool (see gc_inner_obj_book) -- this
        // Object is the sole owner of the buffer, same as
        // free_object's own treatment of mut_string.
        os_free ((void *)obj->value);
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

static void active_root_insert (object_t obj);

static void active_root_inner_insert (otype_t type, void *value)
{
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
    case mut_string:
      {
        // Not pool-tracked, nothing to mark alive.
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
        vector_t v = (vector_t)value;
        for (u16_t i = 0; i < v->size; i++)
          {
            active_root_insert (v->vec[i]);
          }
        insert_value (value);
        break;
      }
    case list:
      {
        ListHead *head = &((list_t)value)->list;
        list_node_t node;
        for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
          active_root_insert (node->obj);
        }

        insert_value (value);
        break;
      }
    case closure_on_heap:
    case closure_on_stack:
      {
        // The closure's own persistent captured-variable storage
        // (env[], sized to frame_size -- see make_closure) must be
        // walked here regardless of *how* this closure was reached
        // (a global binding, nested in a pair/list, or on the active
        // call chain) -- it's the closure's permanent state, not
        // something tied to any one particular invocation.
        closure_t c = (closure_t)value;
        for (u8_t i = 0; i < c->frame_size; i++)
          {
            active_root_insert (&c->env[i]);
          }
        insert_value (value);
        break;
      }
    case bytevector:
    case mut_bytevector:
      {
        // Raw bytes, no further object_t sub-references to walk.
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

  // Delegate the type-specific walk (and marking the *inner* value
  // alive, when there is one) to active_root_inner_insert, so there's
  // a single place that knows how to walk each otype_t. Then mark the
  // outer wrapper itself alive too, since obj_free_pool's liveness
  // check (in collect()) looks up outer object_t pointers.
  active_root_inner_insert (obj->attr.type, obj->value);
  insert_value ((void *)obj);
}

static void active_root_insert_frame (const u8_t *stack, u32_t local, u8_t cnt)
{
  /* printf ("insert frame %d, %d\n", local, cnt); */
  /* getchar (); */
  for (u8_t i = 0; i < cnt; i++)
    {
      object_t obj = (object_t)(stack + local + i * sizeof (Object));

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
              object_t obj = (&((object_t)(stack + closure->local))[i]);
              active_root_inner_insert (obj->attr.type, obj->value);
            }
        }
    }

  // After walking every active call frame (if any -- there may be
  // none, e.g. between top-level forms), `sp` bounds the outermost
  // region: the top-level code's own locals. fp==0 there is the base
  // case, not "nothing exists" -- top-level `define`s/values live
  // directly in [0, sp) on the ordinary stack, with no call prelude to
  // skip (nothing ever "called" the top level). The loop above only
  // ever scans *inside* an active call chain, so this region -- where
  // e.g. `z` in `(define z (func 123))` lives the moment control
  // returns to the top level -- was never scanned as a root at all.
  u8_t top_level_cnt = sp / sizeof (Object);
  active_root_insert_frame (stack, 0, top_level_cnt);

  // Runtime-created globals (top-level `define`s) are roots too --
  // without this, anything reachable only via vm->globals (e.g. a
  // closure bound to a global, per the comment on
  // GLOBAL_REF(VM_GLOBALSEG_SIZE)'s definition site in vm.c) looks
  // unreachable to every collect_inner/collect call below, and gets
  // freed out from under the global that still points to it.
  if (g_current_vm && g_current_vm->globals)
    {
      size_t globals_cnt = GLOBAL_REF (VM_GLOBALSEG_SIZE) / sizeof (Object);
      for (size_t i = 0; i < globals_cnt; i++)
        {
          object_t obj = &g_current_vm->globals[i];
          active_root_inner_insert (obj->attr.type, obj->value);
        }
    }
}

static void clean_active_root ()
{
  /* NOTE: Don't waste time to clean one by one -- the ARN slots get
   * reused from index 0 again on the next gc() cycle, and resetting
   * the tree root is O(1), same as the old list-head reset was. */
  _arn.index = 0;
  RB_INIT (&active_root_tree);
}

static void collect (size_t *count, ListHead *head, bool hurt, bool force)
{
  /* GC algo:
      1. Skip permanent object.
      2. If it 's in active root, it get aged if it' s gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */
  list_node_t node;
  for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
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
  switch (type)
    {
#  define X(tag, ctype, pool) \
    case tag:                 \
      return ((ctype)value)->attr.gc;
      GC_INNER_TYPE_LIST (X)
#  undef X
    case closure_on_heap:
    case closure_on_stack:
      return ((closure_t)value)->attr.gc;
    default:
      PANIC ("Invalid node type %d\n", type);
    }

  return FREE_OBJ; // unreachable, PANIC never returns; silences -Wreturn-type
}

static void set_gc_to_node (otype_t type, void *value, int gc)
{
  switch (type)
    {
#  define X(tag, ctype, pool)          \
    case tag:                          \
      {                                \
        ((ctype)value)->attr.gc = gc; \
        break;                        \
      }
      GC_INNER_TYPE_LIST (X)
#  undef X
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
  /* GC algo:
      1. Skip permanent object.
      2. If it's in active root, it get aged if it's gen-1, keep age if it's
         gen-2.
      3. If it's not in active root, release it.
      4. Collect all gen-2 object in hurt collect.
   */
  list_node_t node;
  for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
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
  size_t cnt = 0;
  list_node_t node;
  for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
    cnt++;
  }
  return cnt;
}

static void release_all_free_objects (ListHead *head, bool force)
{
  if (!SLIST_EMPTY (head))
    {
      list_node_t node = SLIST_FIRST (head);
      while (node)
        {
          list_node_t next_node = SLIST_NEXT(node, next);
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
              object_list_node_recycle (node);
            }
          node = next_node;
        }
    }
}

static void sweep (bool force)

{
#  define X(tag, ctype, pool)             \
    VM_DEBUG ("sweep " #pool "\n");       \
    release_all_free_objects (&pool, force);
  GC_ALL_POOLS_LIST (X)
#  undef X
  VM_DEBUG ("sweep obj\n");
  release_all_free_objects (&obj_free_pool, force);
}

bool gc (const gc_info_t gci)
{
  // Any collection, regardless of what triggered it, counts as
  // "caught up" for the proactive budget-based trigger.
  alloc_since_last_gc = 0;

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

#  define X(tag, ctype, pool) collect_inner (&count, &pool, tag, false, false);
  GC_ALL_POOLS_LIST (X)
#  undef X
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
#  define X(tag, ctype, pool) collect_inner (&count, &pool, tag, true, false);
      GC_ALL_POOLS_LIST (X)
#  undef X
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

// DESIGN NOTE (flagged for the person to reconsider if it turns out to
// matter): setting g_gc_force_teardown here makes gc_clean_cache
// reclaim PERMANENT_OBJ objects too, same as gc_teardown. That's fine
// if each HALT effectively ends an independent script/session (a
// later vm_init_globals for a *new* LEF wouldn't expect anything from
// the *previous* one to still be reachable) -- but if there's ever a
// scenario where "permanent" bindings are meant to survive across
// multiple sequential script loads within the same live process, this
// would incorrectly reclaim them at the first HALT.
void gc_clean_cache (void)
{
  size_t cnt = 0;

  g_gc_force_teardown = true;

#  define X(tag, ctype, pool) collect_inner (&cnt, &pool, tag, false, true);
  GC_ALL_POOLS_LIST (X)
#  undef X

  // free self-contained object in sweep
  sweep (true);

  g_gc_force_teardown = false;
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
#  define X(tag, ctype, pool)                  \
    case tag:                                  \
      {                                        \
        SLIST_INSERT_HEAD (&pool, node, next); \
        break;                                 \
      }
      GC_INNER_TYPE_LIST (X)
#  undef X
    case closure_on_heap:
    case closure_on_stack:
      {
        SLIST_INSERT_HEAD (&closure_free_pool, node, next);
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
#  define X(tag, ctype, pool)                    \
    case tag:                                    \
      {                                          \
        node = get_free_obj_node (&pool);        \
        break;                                   \
      }
      GC_INNER_TYPE_LIST (X)
#  undef X
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

void simple_collect (ListHead *head)
{
  list_node_t node;
  for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
    object_t obj = (object_t)node->obj;

    if (PERMANENT_OBJ != obj->attr.gc)
      {
        obj->attr.gc = FREE_OBJ;
      }
  }
}

// Like simple_collect, but for list_free_pool specifically: a list_t
// owns a secondary heap-allocated ListNode chain (its elements) that
// isn't tracked in any pool of its own. Marking the list_t dead
// without first freeing that chain orphans it the instant this slot
// is reused for a different list (SLIST_INIT overwrites the only
// pointer to the old chain). free_object never touches the .attr.gc
// of the object passed to it directly (only what it owns), so calling
// it here on each element is safe alongside simple_collect's own pass
// over obj_free_pool in gc_try_to_recycle -- neither double-marks the
// other's work.
static void simple_collect_list (ListHead *head)
{
  list_node_t node;
  for (node = SLIST_FIRST (head); node != NULL; node = SLIST_NEXT (node, next))
    {
      list_t l = (list_t)node->obj;

      if (PERMANENT_OBJ == l->attr.gc)
        continue;

      free_list_nodes (l, free_object);
      l->attr.gc = FREE_OBJ;
    }
}

// collect all composite object, including vector, list, pair
// bytevector is not included, since it does not have child objects with
// attr.gc
void gc_try_to_recycle (void)
{
  /* FIXME: The runtime created globals shouldn't be recycled -- see the
   * note on build_active_root: it only walks the current call-frame
   * chain (fp/sp), never vm->globals, so a real gc() run at top level
   * (fp == 0, no active frames) would currently treat every global as
   * unreachable. Until that's fixed, this stays a blunt "anything
   * non-permanent is garbage" pass rather than a real reachability
   * check, which is only safe as long as nothing here is a
   * runtime-created global the script still needs.
   */
  simple_collect (&obj_free_pool);
  simple_collect_list (&list_free_pool);
  simple_collect (&vector_free_pool);
  simple_collect (&pair_free_pool);

  /* NOTE:
   * Closures are not fixed size object, so we have to free it.
   */
  release_all_free_objects (&closure_free_pool, true);
}

// Final teardown, meant to be called exactly once, right before the
// program/process exits (e.g. right before vm_clean(vm)) -- NOT during
// normal execution. Treats every object across every pool as garbage
// -- PERMANENT_OBJ included -- regardless of whether it's still
// technically reachable from the VM stack, and reclaims it through
// the same collect_inner/collect/sweep pipeline a real gc() cycle
// uses, with force=true propagated through every stage (not just
// sweep) -- so, unlike gc_try_to_recycle's simple_collect passes,
// internal structures (a list_t's ListNode chain, a closure's frame,
// etc.) are correctly torn down first, rather than the outer struct
// just being os_free'd out from under them.
//
// This exists because a short-lived script may never allocate enough
// -- or ever hit a real allocation failure -- to cross any of the
// runtime GC triggers (reactive malloc-failure, oln-pool exhaustion,
// or the proactive gc_alloc_budget_exceeded threshold) even once
// during its entire run. Without an explicit final reap, whatever
// garbage such a script produced simply stays allocated until the
// process exits, which is exactly what shows up as a "leak" under
// LeakSanitizer even though nothing is actually wrong with the
// program's logic.
//
// NOTE: same caveat as gc_try_to_recycle -- this has no notion of
// vm->globals being special, so only call this when the program is
// truly finished and nothing (including any runtime-created global)
// needs to survive any longer.
void gc_teardown (void)
{
  size_t count = 0;

  // Override free_object/free_inner_object's own independent
  // PERMANENT_OBJ guards for the duration of this pass -- see
  // g_gc_force_teardown's own comment above for why this is needed.
  g_gc_force_teardown = true;

  // Ensure the active root is empty regardless of prior state, so
  // exist() returns false for everything below -- deliberately never
  // calling build_active_root here.
  clean_active_root ();

#  define X(tag, ctype, pool) collect_inner (&count, &pool, tag, true, true);
  GC_ALL_POOLS_LIST (X)
#  undef X
  collect (&count, &obj_free_pool, false, true);

  // force=true here also physically frees PERMANENT_OBJ pool entries
  // (release_all_free_objects's force bypasses the .attr.gc check
  // entirely) -- intentional for a final teardown: nothing needs to
  // survive after this point, and LeakSanitizer doesn't care whether
  // something was logically "permanent" from the program's own
  // perspective.
  sweep (true);

  g_gc_force_teardown = false;
}

void gc_recycle_current_frame (const u8_t *stack, u32_t local, u32_t sp)
{
#  if defined GC_RECYCLE_CURRENT_FRAME == 1
  size_t size = sizeof (Object);
  size_t cnt = (sp - local) / size;

  for (size_t i = 0; i < cnt; i++)
    {
      object_t obj = (object_t)(stack + local + i * size);

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
            free_object_from_pool (&closure_free_pool, obj->value);
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
#  define X(tag, ctype, pool) SLIST_INIT (&pool);
  GC_ALL_POOLS_LIST (X)
#  undef X
}

void gc_clean (void)
{
  active_nodes_clean ();
  object_list_node_clean ();
}

// remove first find object in LIST head
static void free_object_from_pool (ListHead *head, void *o)
{
  list_node_t node = NULL;
  for (node = SLIST_FIRST(head); node != NULL; node = SLIST_NEXT(node, next)) {
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
