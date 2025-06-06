#ifndef __ANIMULA_LEF_H__
#define __ANIMULA_LEF_H__
/*  Copyright (C) 2020-2021
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

#include "__stdio.h"
#include "debug.h"
#include "memory.h"
#include "os.h"
#include "storage.h"
#include "symbol.h"
#include "types.h"
#include "vos.h"
#ifdef ANIMULA_ZEPHYR
#  include <fs/fs.h>
#  include <kernel.h>
#endif

typedef struct LEF
{
  char sig[3];
  u8_t ver[3];
  u32_t msize;
  u32_t gsize;
  u32_t psize;
  u32_t csize;
  u32_t entry;
  u8_t *body;
  symtab symtab;
} __packed *lef_t;

#define LEF_VERIFY(lef) \
  ((lef)->sig[0] == 'L' && (lef)->sig[1] == 'E' && (lef)->sig[2] == 'F')

#define LEF_MEM(lef)    (&(lef)->body[0])
#define LEF_GLOBAL(lef) (&(lef)->body[(lef)->msize])
#define LEF_PROG(lef)   (&(lef)->body[(lef)->msize + (lef)->gsize])
#define LEF_CLEAN(lef) \
  (&(lef)->body[(lef)->msize + (lef)->gsize + (lef)->psize])
#define LEF_BODY_SIZE(lef) \
  ((lef)->msize + (lef)->gsize + (lef)->psize + (lef)->csize)
#define LEF_SIZE(lef) (sizeof (struct LEF) + LEF_BODY_SIZE (lef))

static inline u16_t lef_get_u16 (u16_t offset, lef_t lef)
{
  u8_t size[2] = {0};

#if defined ANIMULA_BIG_ENDIAN
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

#if defined ANIMULA_BIG_ENDIAN
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

#if defined(ANIMULA_LINUX) || defined(ANIMULA_ZEPHYR)
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

static inline bool file_exist (const char *filename)
{
#if defined ANIMULA_LINUX
  struct stat st = {0};
  return (linux_stat (filename, &st) == 0);
#elif defined ANIMULA_ZEPHYR
  struct fs_dirent entry = {0};
  return (zephyr_stat (filename, &entry) == 0);
#endif
}

void store_lef (lef_t lef, size_t offset);
void free_lef (lef_t lef);
lef_t load_lef_from_file (const char *filename);
lef_t load_lef_from_uart (void);
lef_t load_lef_from_flash (size_t offset);

#endif // End of __ANIMULA_LEF_H__
