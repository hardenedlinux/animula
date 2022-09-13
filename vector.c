/*  Copyright (C) 2020
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

#include "vector.h"

u8_t vector_ref (vector_t vec, u8_t index)
{
  /* TODO
     NOTE:
     * The return value must be u8_t which is the address in ss, otherwise we
     can't push it into the dynamic stack.
     * So the fetched value must be stored into ss.
     */

  return 0;
}

void vector_set (vector_t vec, u8_t index, object_t value)
{
  // TODO
}
