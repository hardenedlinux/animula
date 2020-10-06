#ifndef __LAMBDACHIP_GC_H__
#define __LAMBDACHIP_GC_H__
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

#include "debug.h"
#include "memory.h"
#include "rbtree.h"
#include "types.h"

typedef enum gc_obj_type
{
  gc_object,
  gc_pair,
  gc_vector,
  gc_continuation,
  gc_list,
  gc_closure,
  gc_procedure,
  gc_obj_list
} gobj_t;

typedef struct ActiveRoot ActiveRoot;
typedef struct ActiveRootNode ActiveRootNode;

struct ActiveRootNode
{
  RB_ENTRY (ActiveRootNode) entry;
  void *value;
};

static inline bool active_root_compare (ActiveRootNode *a, ActiveRootNode *b)
{
  return (b->value - a->value);
}

static inline obj_list_t get_free_obj_node (obj_list_head_t *lst)
{
  obj_list_t node = NULL;
  SLIST_FOREACH (node, lst, next)
  {
    /* NOTE: when it's free, gc is 0.
     */
    if (!node->obj->attr.gc)
      {
        // printf ("pool obj %p gc is %d\n", node->obj, node->obj->attr.gc);
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

#define MALLOC_OBJ_FROM_POOL(lst)                 \
  do                                              \
    {                                             \
      obj_list_t node = get_free_obj_node (&lst); \
      if (node)                                   \
        ret = node->obj;                          \
    }                                             \
  while (0)

#define RECYCLE_OBJ(lst)                  \
  do                                      \
    {                                     \
      SLIST_FOREACH (node, &lst, next)    \
      {                                   \
        if (node->obj == obj)             \
          {                               \
            ((object_t)obj)->attr.gc = 0; \
            return;                       \
          }                               \
      }                                   \
    }                                     \
  while (0)

#define NEXT_FP() (*((u32_t *)(stack + fp + 4)))

#define FREE_OBJECT(head, obj)                           \
  do                                                     \
    {                                                    \
      obj_list_t node = NULL;                            \
      SLIST_FOREACH (node, head, next)                   \
      {                                                  \
        if (node->obj == obj)                            \
          {                                              \
            SLIST_REMOVE (head, node, ObjectList, next); \
            os_free (node->obj);                         \
            os_free (node);                              \
            break;                                       \
          }                                              \
      }                                                  \
    }                                                    \
  while (0)

#define __FREE_OBJECTS(head, force)                      \
  do                                                     \
    {                                                    \
      obj_list_t node = NULL;                            \
      obj_list_t prev = NULL;                            \
      SLIST_FOREACH (node, head, next)                   \
      {                                                  \
        if (prev)                                        \
          {                                              \
            SLIST_REMOVE (head, prev, ObjectList, next); \
            free_object ((object_t)prev->obj);           \
            prev->obj = NULL;                            \
            os_free (prev);                              \
            prev = NULL;                                 \
          }                                              \
        if (force || (0 == node->obj->attr.gc))          \
          {                                              \
            prev = node;                                 \
          }                                              \
      }                                                  \
    }                                                    \
  while (0)

#define FREE_OBJECTS(head)       __FREE_OBJECTS (head, false)
#define FORCE_FREE_OBJECTS(head) __FREE_OBJECTS (head, true)

void gc_init (void);
bool gc (const gc_info_t gci);
void gc_clean_cache (void);
void *gc_pool_malloc (gobj_t type);
void gc_book (gobj_t type, object_t obj);
void gc_try_to_recycle (void);
#endif // End of __LAMBDACHIP_GC_H__
