#ifndef __LAMBDACHIP_OS_H__
#define __LAMBDACHIP_OS_H__
/*  Copyright (C) 2019,2020
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
#include <zephyr.h>
#include <sys/printk.h>
#define __printk printk
#define get_platform_info() CONFIG_BOARD
#include <kernel.h>
#define __malloc k_malloc
#define __free   k_free
#include <string.h>
#define __memset memset
#define __memcpy memcpy
#include <console/console.h>
#define __getchar console_getchar
#define __getline console_getline
#include <device.h>
#include <drivers/flash.h>

#if BIG_ENDIAN
#define LAMBDACHIP_BIG_ENDIAN
#else
#define LAMBDACHIP_LITTLE_ENDIAN
#endif

#else
#error "Please specify a platform!"
#endif

#endif // End of __LAMBDACHIP_OS_H__
