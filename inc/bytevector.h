#ifndef __ANIMULA_BYTEVECTOR_H
#define __ANIMULA_BYTEVECTOR_H

/*  Copyright (C) 2020-2021
 *         Rafael Lee <rafaellee.img@gmail.com>
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

#include "types.h"

object_t _make_bytevector (vm_t vm, object_t ret, object_t count,
                           object_t datum);
object_t _bytevector_length (vm_t vm, object_t ret, object_t bv);
object_t _bytevector_u8_ref (vm_t vm, object_t ret, object_t bv,
                             object_t index);
object_t _bytevector_u8_set (vm_t vm, object_t ret, object_t bv, object_t index,
                             object_t datum);
object_t _bytevector_copy (vm_t vm, object_t ret, object_t bv, object_t start,
                           object_t end);
object_t _bytevector_copy_overwrite (vm_t vm, object_t ret, object_t dst,
                                     object_t index, object_t src,
                                     object_t src_start, object_t src_end);
object_t _bytevector_append (vm_t vm, object_t ret, object_t bv0, object_t bv1);

#endif /* __ANIMULA_BYTEVECTOR_H */