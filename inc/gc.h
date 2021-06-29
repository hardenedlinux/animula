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

#define GC()                                                               \
  do                                                                       \
    {                                                                      \
      os_printk ("oh GC?!\n");                                             \
      GCInfo gci                                                           \
        = {.fp = vm->fp, .sp = vm->sp, .stack = vm->stack, .hurt = false}; \
      gc (&gci);                                                           \
    }                                                                      \
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

#define NEW_OBJ(type)                                  \
  ({                                                   \
    object_t obj = NULL;                               \
    do                                                 \
      {                                                \
        while (false == is_oln_available ())           \
          {                                            \
            GC ();                                     \
          }                                            \
        if (unbooked == type)                          \
          {                                            \
            obj = lambdachip_new_object (0, true);     \
          }                                            \
        else                                           \
          {                                            \
            obj = lambdachip_new_object (type, false); \
          }                                            \
        if (obj)                                       \
          break;                                       \
        GC ();                                         \
      }                                                \
    while (1);                                         \
    obj;                                               \
  })

#define NEW_UNBOOKED_OBJ() NEW_OBJ (unbooked)

#define NEW_OBJ_LIST_NODE()                   \
  ({                                          \
    obj_list_t ol = NULL;                     \
    do                                        \
      {                                       \
        ol = lambdachip_new_obj_list_node (); \
        if (ol)                               \
          {                                   \
            gc_oln_book (ol);                 \
            break;                            \
          }                                   \
        GC ();                                \
      }                                       \
    while (1);                                \
    ol;                                       \
  })

#define NEW(t)                       \
  ({                                 \
    t##_t obj = NULL;                \
    do                               \
      {                              \
        obj = lambdachip_new_##t (); \
        if (obj)                     \
          break;                     \
        GC ();                       \
      }                              \
    while (1);                       \
    obj->attr.type = (t);            \
    obj->attr.gc = 1;                \
    obj;                             \
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
  int cnt;
  obj_list_t oln[PRE_OLN];
};

static inline int active_root_compare (ActiveRootNode *a, ActiveRootNode *b)
{
  return ((uintptr_t)b->value - (uintptr_t)a->value);
}

static inline obj_list_t get_free_obj_node (obj_list_head_t *lst)
{
  obj_list_t node = NULL;
  SLIST_FOREACH (node, lst, next)
  {
    /* NOTE: when it's free, gc is 0.
     */
    if (FREE_OBJ == node->obj->attr.gc)
      {
        // os_printk ("pool obj %p gc is %d\n", node->obj, node->obj->attr.gc);
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

#define MALLOC_OBJ_FROM_POOL(lst)                   \
  do                                                \
    {                                               \
      obj_list_t node = get_free_obj_node (&(lst)); \
      if (node)                                     \
        ret = node->obj;                            \
    }                                               \
  while (0)

#define RECYCLE_OBJ(lst)                  \
  do                                      \
    {                                     \
      obj_list_t node = NULL;             \
      SLIST_FOREACH (node, &(lst), next)  \
      {                                   \
        if (node->obj == obj)             \
          {                               \
            ((object_t)obj)->attr.gc = 0; \
            break;                        \
          }                               \
      }                                   \
    }                                     \
  while (0)

#define FREE_OBJECT(head, o)                             \
  do                                                     \
    {                                                    \
      obj_list_t node = NULL;                            \
      SLIST_FOREACH (node, (head), next)                 \
      {                                                  \
        if (node->obj == (o))                            \
          {                                              \
            SLIST_REMOVE (head, node, ObjectList, next); \
            os_free (node->obj);                         \
            node->obj = NULL;                            \
            obj_list_node_recycle (node);                \
            break;                                       \
          }                                              \
      }                                                  \
    }                                                    \
  while (0)

#define __FREE_OBJECTS(head, force)             \
  do                                            \
    {                                           \
      obj_list_t node = NULL;                   \
      SLIST_FOREACH (node, (head), next)        \
      {                                         \
        if (force || (0 == node->obj->attr.gc)) \
          {                                     \
            free_object ((object_t)node->obj);  \
          }                                     \
      }                                         \
    }                                           \
  while (0)

#define FREE_OBJECTS(head)       __FREE_OBJECTS (head, false)
#define FORCE_FREE_OBJECTS(head) __FREE_OBJECTS (head, true)

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

void gc_init (void);
bool gc (const gc_info_t gci);
void gc_clean_cache (void);
void *gc_pool_malloc (otype_t type);
void gc_oln_book (obj_list_t node);
void gc_book (otype_t type, object_t obj, obj_list_t node);
void gc_try_to_recycle (void);
bool is_oln_available (void);
obj_list_t oln_alloc (void);
void gc_clean (void);
#endif // End of __LAMBDACHIP_GC_H__
