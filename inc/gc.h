#ifndef __ANIMULA_GC_H__
#define __ANIMULA_GC_H__
/*  Copyright (C) 2023
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

// include obg_gc.h or tiny_gc.h by macro
#ifdef USE_TINY_GC
#  include "tiny_gc.h"
#  define ANIMULA_GC_INIT()       \
    do                            \
      {                           \
        GC_INIT ();               \
        GC_enable ();             \
        GC_enable_incremental (); \
      }                           \
    while (0);
#  define GC() GC_gcollect ()
#  define GC_CLEAN()                                         do { } while (0)
// GC_MALLOC was provided by tiny_gc.h
// NOTE: do{}while(0) (rather than expanding to nothing) so these are
// safe to use as the body of an `if (...) gc_obj_book(...);` without
// braces -- expanding to nothing turns that into `if (...) ;`, which
// -Wempty-body flags and -Werror then turns into a build failure.
#  define gc_obj_book(...)                                do { } while (0)
#  define gc_inner_obj_book(...)                           do { } while (0)
#  define gc_recycle_current_frame(...)                    do { } while (0)
#  define gc_clean_cache()                                  do { } while (0)
#  define gc_try_to_recycle()                                do { } while (0)
#  define gc_bind_vm(vm)                                     do { } while (0)
#  define gc_alloc_budget_exceeded()    false // tiny gc already collects incrementally
#  define gc_teardown()                                      do { } while (0)
#  define object_list_node_available()  1    // always true
#  define gc_pool_malloc(te)            NULL // always NULL
#else
#  include "obg_gc.h"
#  define ANIMULA_GC_INIT() gc_init ()
#  define GC()              ODB_GC ()
#  define GC_MALLOC(n)      ODB_GC_MALLOC (n)
#  define GC_CLEAN()        gc_clean ()
#endif

#endif // End of __ANIMULA_GC_H__
