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
#  define GC_CLEAN()
// GC_MALLOC was provided by tiny_gc.h
#  define gc_inner_obj_book                  // tiny gc doesn't need it
#  define gc_inner_obj_book                  // tiny gc doesn't need it
#  define gc_recycle_current_frame(...)      // tiny gc doesn't need it
#  define gc_clean_cache()                   // tiny gc doesn't need it
#  define gc_try_to_recycle()                // tiny gc doesn't need it
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
