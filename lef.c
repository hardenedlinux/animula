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

void store_lef(lef_t lef, size_t offset)
{
  size_t head_size = sizeof(struct LEF);
  os_flash_write((const void*)lef, offset, head_size);
  os_flash_write((const void*)lef->body, head_size, LEF_SIZE(lef));
}

void free_lef(lef_t lef)
{
  os_free(lef->body);
  os_free(lef);
}

lef_t load_lef_from_uart(vm_t vm)
{
  lef_t lef = (lef_t)os_malloc(sizeof(struct LEF));

  for(int i = 0; i < 3; i++)
    lef->sig[i] = getchar();

  if(!LEF_VERIFY(lef))
    {
      uart_drop_rest_data();
      os_printk("Wrong LEF file, please check it then re-upload!\n");
      return NULL;
    }

  for(int i = 0; i < 3; i++)
    lef->ver[i] = getchar();

  lef->msize = uart_get_u32();
  lef->psize = uart_get_u32();
  lef->csize = uart_get_u32();

  u32_t size = LEF_BODY_SIZE(lef);
  lef->body = (u8_t*)os_malloc(size);

  for(int i = 0; i < size; i++)
    {
      u8_t ch = getchar();

      if(!ch)
        {
          os_printk("Invalid LEF, wrong size: %d!\n", i);
          os_free(lef);
          return NULL;
        }

      os_printk("Upload: %%%d\n", (i*100)/size);
      lef->body[i] = ch;
    }

  os_printk("Done\n");

  return lef;
}
