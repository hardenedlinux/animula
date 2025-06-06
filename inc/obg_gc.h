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

extern vm_t vm; // magic to make gcc happy

#define ODB_GC()                              \
  do                                          \
    {                                         \
      GCInfo gci = {.fp = vm->fp,             \
                    .sp = vm->sp,             \
                    .stack = vm->stack,       \
                    .hurt = ANIMULA_GC_HURT}; \
      gc (&gci);                              \
    }                                         \
  while (0)

#define ODB_GC_MALLOC(size)             \
  ({                                    \
    void *ret = NULL;                   \
    do                                  \
      {                                 \
        ret = (void *)os_malloc (size); \
        if (ret)                        \
          break;                        \
        GC ();                          \
      }                                 \
    while (1);                          \
    ret;                                \
  })

typedef struct ActiveRoot ActiveRoot;
typedef struct ActiveRootNode ActiveRootNode;

struct ActiveRootNode
{
  RB_ENTRY (ActiveRootNode) entry;
  void *value;
};

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

static void object_list_node_recycle (list_node_t node);
static void free_object_from_pool (ListHead *head, object_t o);
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
#endif // End of __ANIMULA_GC_H__
