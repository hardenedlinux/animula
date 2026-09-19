#ifndef __ANIMULA_VOS_H__
#define __ANIMULA_VOS_H__

/*
 *  Copyright (C) 2026 HardenedLinux Community
 *        Nala Ginrut <roy@hardenedlinux.org>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or  (at your option) any later version.
 *
 *  Animula is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  VOS platform selector.
 *
 *  This is the single entry point by which the VM and Primitive layers
 *  reach platform capabilities: it dispatches to one per-platform OAL
 *  header that maps the os_* names onto that platform's real APIs.  No
 *  architecture/board/linker selection happens here, and no weak symbols
 *  are used.  VOS only expresses semantics Animula actually needs; Linux
 *  is only ONE OAL target -- never the definition of the contract itself.
 */

#if defined ANIMULA_ZEPHYR
#  include "vos/oal/zephyr/vos.h"
#elif defined ANIMULA_LINUX
#  include "vos/oal/linux/vos.h"
#else
#  error "Please specify a platform!"
#endif

#endif /* __ANIMULA_VOS_H__ */
