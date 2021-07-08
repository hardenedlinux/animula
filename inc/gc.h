#ifndef __LAMBDACHIP_GC_H__
#define __LAMBDACHIP_GC_H__
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

#include "debug.h"
#include "memory.h"
#include "rbtree.h"
#include "types.h"

#define PERMANENT_OBJ 3
#define GEN_2_OBJ     2
#define GEN_1_OBJ     1
#define FREE_OBJ      0

#define GC()                                     \
  do                                             \
    {                                            \
      printf ("GC in!\n");                       \
      GCInfo gci = {.fp = vm->fp,                \
                    .sp = vm->sp,                \
                    .stack = vm->stack,          \
                    .hurt = LAMBDACHIP_GC_HURT}; \
      gc (&gci);                                 \
      printf ("GC out!\n");                      \
    }                                            \
  while (0)

#define GC_MALLOC(size)                 \
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

#define NEW_OBJ(t)                        \
  ({                                      \
    object_t obj = NULL;                  \
    do                                    \
      {                                   \
        if (0 == oln_available ())        \
          {                               \
            printf ("NEW_OBJ no oln!\n"); \
            GC ();                        \
            continue;                     \
          }                               \
        obj = lambdachip_new_object (t);  \
        if (obj)                          \
          break;                          \
        GC ();                            \
      }                                   \
    while (1);                            \
    obj;                                  \
  })

#define NEW_LIST_NODE()                   \
  ({                                      \
    obj_list_t ol = NULL;                 \
    do                                    \
      {                                   \
        ol = lambdachip_new_list_node (); \
        if (ol)                           \
          {                               \
            break;                        \
          }                               \
        GC ();                            \
      }                                   \
    while (1);                            \
    ol;                                   \
  })

#define NEW(t)                        \
  ({                                  \
    t##_t x = NULL;                   \
    do                                \
      {                               \
        if (0 == oln_available ())    \
          {                           \
            printf ("NEW no oln!\n"); \
            GC ();                    \
            continue;                 \
          }                           \
        x = lambdachip_new_##t ();    \
        if (x)                        \
          {                           \
            gc_book (t, x, true);     \
            break;                    \
          }                           \
        GC ();                        \
      }                               \
    while (1);                        \
    x;                                \
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
  obj_list_t oln[PRE_OLN];
};

static inline int active_root_compare (ActiveRootNode *a, ActiveRootNode *b)
{
  // NOTE: Don't use uintptr_t for minus comparison
  return ((intptr_t)b->value - (intptr_t)a->value);
}

static inline obj_list_t get_free_obj_node (obj_list_head_t *lst)
{
  obj_list_t node = NULL;

  SLIST_FOREACH (node, lst, next)
  {
    /* NOTE: when it's free, gc is 0.
     */
    //    printf ("I'm in free list\n");
    if (FREE_OBJ == node->obj->attr.gc)
      {
        node->obj->attr.gc = 1; // allocated, as the 1st generation
        break;
      }
  }

  return node;
}

/* static inline obj_list_t get_free_node (obj_list_head_t *lst) */
/* { */
/*   obj_list_t node = NULL; */

/*   if (!SLIST_EMPTY (lst)) */
/*     { */
/*       node = SLIST_FIRST (lst); */
/*       SLIST_REMOVE (lst, node, ObjectList, next); */
/*     } */

/*   return node; */
/* } */

#define RECYCLE_OBJ(lst, obj)            \
  do                                     \
    {                                    \
      obj_list_t node = NULL;            \
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

#define FREE_OBJECT(head, o)                             \
  do                                                     \
    {                                                    \
      obj_list_t node = NULL;                            \
      SLIST_FOREACH (node, (head), next)                 \
      {                                                  \
        if (node->obj == (o))                            \
          {                                              \
            os_free (node->obj);                         \
            printf ("freed %p\n", node->obj);            \
            node->obj = NULL;                            \
            SLIST_REMOVE (head, node, ObjectList, next); \
            obj_list_node_recycle (node);                \
            break;                                       \
          }                                              \
      }                                                  \
    }                                                    \
  while (0)

#define FREE_LIST_PRINT(head)                               \
  do                                                        \
    {                                                       \
      obj_list_t node = NULL;                               \
      os_printk ("--------------------------\n");           \
      SLIST_FOREACH (node, (head), next)                    \
      {                                                     \
        os_printk ("node: %p, obj: %p\n", node, node->obj); \
      }                                                     \
      os_printk ("--------------------------\n");           \
    }                                                       \
  while (0)

void free_object (object_t obj);
void gc_init (void);
bool gc (const gc_info_t gci);
void gc_clean_cache (void);
void *gc_pool_malloc (otype_t type);
void gc_book (otype_t type, void *obj, bool non_obj);
void gc_try_to_recycle (void);
void gc_recycle_current_frame (const u8_t *stack, u32_t local, u32_t sp);
size_t oln_available (void);
obj_list_t oln_alloc (void);
void gc_clean (void);
#endif // End of __LAMBDACHIP_GC_H__
