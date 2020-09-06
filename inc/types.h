#ifndef __LAMBDACHIP_TYPES_H
#define __LAMBDACHIP_TYPES_H
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

#if defined LAMBDACHIP_ZEPHYR
#  ifndef CONFIG_HEAP_MEM_POOL_SIZE
#    error "You must define CONFIG_HEAP_MEM_POOL_SIZE for Zephyr!"
#  endif
#  include <stddef.h>
#  include <zephyr/types.h>
#  define bool _Bool
typedef __u16_t reg_t;

#elif defined LAMBDACHIP_LINUX
#  define CONFIG_HEAP_MEM_POOL_SIZE 90000
#  include "__types.h"
#  include <stdint.h>

#else
#  define CONFIG_HEAP_MEM_POOL_SIZE 90000
#  include "__types.h"
#endif

#if defined __GNUC__
#  ifndef __packed
#    define __packed __attribute__ ((packed))
#  endif
#endif

#endif // End of __LAMBDACHIP_TYPES_H;
