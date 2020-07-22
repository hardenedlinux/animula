#ifndef __LAMBDACHIP_STDIO_H__
#define __LAMBDACHIP_STDIO_H__
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
#include "os.h"

#if defined LAMBDACHIP_ZEPHYR
#  define LINE_BUF_SIZE CONFIG_CONSOLE_GETCHAR_BUFSIZE
#else
#  define LINE_BUF_SIZE 80 // enough for one text line on VGA
#endif

char *read_line (const char *prompt);
void stdio_init (void);
void stdio_clean (void);
#endif // End of __LAMBDACHIP_STDIO_H__
