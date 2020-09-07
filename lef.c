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

#include "lef.h"

void store_lef (lef_t lef, size_t offset)
{
  size_t head_size = sizeof (struct LEF);

  os_flash_write ((const void *)lef, offset, head_size);
  os_flash_write ((const void *)lef->body, head_size, LEF_BODY_SIZE (lef));
}

void free_lef (lef_t lef)
{
  os_free (lef->body);
  os_free (lef);
}

#if defined LAMBDACHIP_ZEPHYR
lef_t load_lef_from_flash (size_t offset)
{
  lef_t lef = (lef_t)os_malloc (sizeof (struct LEF));

  for (int i = 0; i < 3; i++)
    os_flash_read (lef->ver, i, 4);

  os_flash_read (&lef->msize, 4, 4);
  os_flash_read (&lef->psize, 7, 4);
  os_flash_read (&lef->csize, 11, 4);

  u32_t size = LEF_BODY_SIZE (lef);
  lef->body = (u8_t *)os_malloc (size);

  os_flash_read (lef->body, 12, size);
  lef->entry = lef_entry (lef);
  os_printk ("Done\n");
}

lef_t load_lef_from_uart ()
{
  lef_t lef = (lef_t)os_malloc (sizeof (struct LEF));

  for (int i = 0; i < 3; i++)
    lef->sig[i] = os_getchar ();

  if (!LEF_VERIFY (lef))
    {
      uart_drop_rest_data ();
      os_printk ("Wrong LEF file, please check it then re-upload!\n");
      exit (-1);
    }

  for (int i = 0; i < 3; i++)
    lef->ver[i] = os_getchar ();

  lef->msize = uart_get_u32 ();
  lef->psize = uart_get_u32 ();
  lef->csize = uart_get_u32 ();

  u32_t size = LEF_BODY_SIZE (lef);
  lef->body = (u8_t *)os_malloc (size);

  for (u32_t i = 0; i < size; i++)
    {
      u8_t ch = os_getchar ();
      os_printk ("Upload: %%%d\n", (i * 100) / size);
      lef->body[i] = ch;
    }

  lef->entry = lef_entry (lef);
  os_printk ("Done\n");

  return lef;
}
#endif

lef_t load_lef_from_file (const char *filename)
{
  lef_t lef = NULL;

#if defined LAMBDACHIP_LINUX
  if (!file_exist (filename))
    {
      os_printk ("File \"%s\" doesn't exist!\n", filename);
      exit (-1);
    }

  int fp;
  lef = (lef_t)os_malloc (sizeof (struct LEF));
  int fd = os_open_input_file (filename);
  os_read (fd, lef->sig, 3);

  if (!LEF_VERIFY (lef))
    {
      os_printk ("Wrong LEF file, please check it then re-upload!\n");
      exit (-1);
    }

  os_read (fd, lef->ver, 3);
  os_read_u32 (fd, &lef->msize);
  os_read_u32 (fd, &lef->psize);
  os_read_u32 (fd, &lef->csize);

  u32_t size = LEF_BODY_SIZE (lef);
  lef->body = (u8_t *)os_malloc (size);

  os_read (fd, lef->body, size);
  lef->entry = lef_entry (lef);

  VM_DEBUG ("Done\n");

#else
  os_printk ("The current platform %s doesn't support filesystem!\n",
             get_platform_info ());
#endif

  return lef;
}
