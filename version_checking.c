/*  Copyright (C) 2020-2021
 *        Rafael Lee <rafaellee.img@gmail.com>
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

#ifdef ANIMULA_ZEPHYR
#  include <version.h>
// match zephyr v2.7.0
#  if KERNELVERSION != 0x2070000
#    error version does not match
#  endif
#endif /* ANIMULA_ZEPHYR */
