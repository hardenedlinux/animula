#ifndef __LAMBDACHIP_LEF_H__
#define __LAMBDACHIP_LEF_H__
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

#include "memory.h"
#include "types.h"
#include "debug.h"
#include "storage.h"
#include "__stdio.h"

typedef struct LEF
{
  char sig[3];
  u8_t ver[3];
  u32_t msize;
  u32_t psize;
  u32_t csize;
  u8_t *body;
} __packed *lef_t;

#define LEF_VERIFY(lef)                         \
  ((lef)->sig[0] == 'L' &&                      \
   (lef)->sig[1] == 'E' &&                      \
   (lef)->sig[2] == 'F')

#define LEF_MEM(lef) (&(lef)->body[0])
#define LEF_PROG(lef) (&(lef)->body[(lef)->msize])
#define LEF_CLEAN(lef) (&(lef)->body[(lef)->psize])
#define LEF_BODY_SIZE(lef) ((lef)->msize + (lef)->psize + (lef)->csize)
#define LEF_SIZE(lef) (sizeof(struct LEF) + LEF_BODY_SIZE(lef))

#if defined LAMBDACHIP_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
static inline bool file_exist (const char* filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}
#endif

void store_lef(lef_t lef, size_t offset);
void free_lef(lef_t lef);
lef_t load_lef_from_uart(void);
lef_t load_lef_from_file(const char* filename);
#endif // End of __LAMBDACHIP_LEF_H__
