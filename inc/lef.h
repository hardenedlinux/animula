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

#include "__stdio.h"
#include "debug.h"
#include "memory.h"
#include "storage.h"
#include "symbol.h"
#include "types.h"

typedef struct LEF
{
  char sig[3];
  u8_t ver[3];
  u32_t msize;
  u32_t psize;
  u32_t csize;
  u32_t entry;
  u8_t *body;
  symtab symtab;
} __packed *lef_t;

#define LEF_VERIFY(lef) \
  ((lef)->sig[0] == 'L' && (lef)->sig[1] == 'E' && (lef)->sig[2] == 'F')

#define LEF_MEM(lef)       (&(lef)->body[0])
#define LEF_PROG(lef)      (&(lef)->body[(lef)->msize])
#define LEF_CLEAN(lef)     (&(lef)->body[(lef)->psize])
#define LEF_BODY_SIZE(lef) ((lef)->msize + (lef)->psize + (lef)->csize)
#define LEF_SIZE(lef)      (sizeof (struct LEF) + LEF_BODY_SIZE (lef))

static inline u16_t lef_get_u16 (u16_t offset, lef_t lef)
{
  u8_t size[2] = {0};

#if defined LAMBDACHIP_BIG_ENDIAN
  size[0] = lef->body[offset + 0];
  size[1] = lef->body[offset + 1];
#else
  size[1] = lef->body[offset + 0];
  size[0] = lef->body[offset + 1];
#endif
  return *((u16_t *)size);
}

static inline u32_t lef_entry (u16_t offset, lef_t lef)
{
  u8_t entry[4] = {0};

#if defined LAMBDACHIP_BIG_ENDIAN
  entry[0] = lef->body[offset + 0];
  entry[1] = lef->body[offset + 1];
  entry[2] = lef->body[offset + 2];
  entry[3] = lef->body[offset + 3];
#else
  entry[3] = lef->body[offset + 0];
  entry[2] = lef->body[offset + 1];
  entry[1] = lef->body[offset + 2];
  entry[0] = lef->body[offset + 3];
#endif
  return *((u32_t *)entry);
}

#if defined LAMBDACHIP_LINUX
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
static inline bool file_exist (const char *filename)
{
  struct stat st;
  return (stat (filename, &st) == 0);
}
#endif

void store_lef (lef_t lef, size_t offset);
void free_lef (lef_t lef);
lef_t load_lef_from_file (const char *filename);
lef_t load_lef_from_uart (void);
lef_t load_lef_from_flash (size_t offset);

#endif // End of __LAMBDACHIP_LEF_H__
