#ifndef __OBG_GC_H__
#define __OBG_GC_H__
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

#include "debug.h"
#include "memory.h"
#include "rbtree.h"
#include "types.h"

// NOTE: the real VM state is threaded through the codebase as an
// explicit `vm_t vm` parameter, not a global -- every VM function
// shadows any global literally named `vm`, and vm.c has no reason to
// know about this GC backend's internals anyway. This global and its
// setter (gc_bind_vm, see below and gc.c) are entirely owned by the
// obg GC backend: they exist only so the ODB_GC()/GC_MALLOC macros
// below -- used from call sites with no vm parameter of their own,
// e.g. inside object.c's allocation functions -- have a way to reach
// the current VM's stack/fp/sp.
//
// The person wiring up a VM needs to call gc_bind_vm(vm) once, right
// after creating/initializing it (e.g. in main.c, after vm_init(vm)).
// Under the tiny gc backend, gc_bind_vm is a no-op (see gc.h), so it's
// safe to call unconditionally regardless of which backend is active.
extern vm_t g_current_vm;
void gc_bind_vm (vm_t vm);

// Set by vm_load_lef/vm.c (GLOBAL_SET(VM_GLOBALSEG_SIZE, lef->gsize)) --
// the byte size of vm->globals. build_active_root needs this to know
// how many Objects to walk when treating runtime-created globals as
// GC roots.
extern GLOBAL_DEF (size_t, VM_GLOBALSEG_SIZE);

#define ODB_GC_EX(is_hurt)                       \
  do                                             \
    {                                            \
      GCInfo gci = {.fp = g_current_vm->fp,      \
                    .sp = g_current_vm->sp,      \
                    .stack = g_current_vm->stack,\
                    .hurt = (is_hurt)};           \
      gc (&gci);                                  \
    }                                             \
  while (0)

// A normal, non-hurt collect: protect gen-2 objects, only release the
// unreachable and let gen-1 age.
#define ODB_GC() ODB_GC_EX (false)

#define ODB_GC_MALLOC(size)              \
  ({                                     \
    void *ret = NULL;                    \
    bool hurt = false;                   \
    do                                   \
      {                                  \
        ret = (void *)os_malloc (size);  \
        if (ret)                         \
          break;                         \
        /* NOTE: the first retry is a normal collect. If we're still  \
         * out of memory after that, escalate to a hurt collect that  \
         * also sacrifices gen-2 objects -- this is what makes "hurt" \
         * an actual runtime signal instead of a fixed compile-time   \
         * value.                                                     \
         */                              \
        ODB_GC_EX (hurt);                \
        hurt = true;                     \
      }                                  \
    while (1);                           \
    ret;                                 \
  })

typedef struct ActiveRoot ActiveRoot;
typedef struct ActiveRootNode ActiveRootNode;

struct ActiveRootNode
{
  RB_ENTRY (ActiveRootNode) entry;
  void *value;
};

// Defines `struct ActiveRoot { struct ActiveRootNode *rbh_root; }`.
// This was previously missing, leaving `struct ActiveRoot` an incomplete
// type -- which is why the RB tree generated below via
// RB_GENERATE_STATIC was never actually usable and gc.c fell back to a
// hand-rolled O(n) linear list instead.
RB_HEAD (ActiveRoot, ActiveRootNode);

struct Pre_ARN
{
  int index;
  ActiveRootNode *arn[PRE_ARN];
};

struct Pre_OLN
{
  int index;
  list_node_t oln[PRE_OLN];
};

static inline int active_root_compare (ActiveRootNode *a, ActiveRootNode *b)
{
  // NOTE: Don't use uintptr_t for minus comparison
  return ((intptr_t)b->value - (intptr_t)a->value);
}

static inline list_node_t get_free_obj_node (ListHead *lst)
{
  list_node_t node = NULL;

  SLIST_FOREACH (node, lst, next)
  {
    /* NOTE: when it's free, gc is 0.
     */
    if (FREE_OBJ == node->obj->attr.gc)
      {
        node->obj->attr.gc = 1; // allocated, as the 1st generation
        break;
      }
  }

  return node;
}

/* static inline list_node_t get_free_node (ListHead *lst) */
/* { */
/*   list_node_t node = NULL; */

/*   if (!SLIST_EMPTY (lst)) */
/*     { */
/*       node = SLIST_FIRST (lst); */
/*       SLIST_REMOVE (lst, node, ListNode, next); */
/*     } */

/*   return node; */
/* } */

#define RECYCLE_OBJ(lst, obj)            \
  do                                     \
    {                                    \
      list_node_t node = NULL;           \
      SLIST_FOREACH (node, &(lst), next) \
      {                                  \
        if (node->obj == obj)            \
          {                              \
            obj->attr.gc = FREE_OBJ;     \
            break;                       \
          }                              \
      }                                  \
    }                                    \
  while (0)

#define FREE_LIST_PRINT(head)                                         \
  do                                                                  \
    {                                                                 \
      list_node_t node = NULL;                                        \
      os_printk ("^^^^^^^^^^^^^^^^^^^^^^^^^^\n");                     \
      SLIST_FOREACH (node, (head), next)                              \
      {                                                               \
        os_printk ("node: %p, obj: %p, value: %p\n", node, node->obj, \
                   node->obj->value);                                 \
      }                                                               \
      os_printk ("vvvvvvvvvvvvvvvvvvvvvvvvvv\n");                     \
    }                                                                 \
  while (0)

void free_object (object_t obj);
void gc_init (void);
bool gc (const gc_info_t gci);
void gc_clean_cache (void);
void *gc_pool_malloc (otype_t type);
void gc_inner_obj_book (otype_t t, void *obj);
void gc_obj_book (void *obj);
void gc_try_to_recycle (void);
void gc_recycle_current_frame (const u8_t *stack, u32_t local, u32_t sp);
size_t object_list_node_available (void);
list_node_t object_list_node_alloc (void);
void gc_clean (void);
bool gc_alloc_budget_exceeded (void);
void gc_teardown (void);
#endif // End of __ANIMULA_GC_H__
