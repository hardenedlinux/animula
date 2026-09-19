/*  Copyright (C) 2026 HardenedLinux Community
 *        Nala Ginrut <roy@hardenedlinux.org>
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
// match Zephyr v4.4.2
#if KERNELVERSION != 0x4040200
#error version does not match
#  endif
#endif /* ANIMULA_ZEPHYR */
