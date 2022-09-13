/*
 *  Copyright (C) 2020-2021
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

#include "os.h"
#ifdef ANIMULA_ZEPHYR
#  include "vos/drivers/file_operation.h"
#  include <disk/disk_access.h>
#  include <kernel.h>
#  include <sys/printk.h>
#  include <zephyr.h>

#  define MAX_PATH_LEN 128
#  define DISK_PDRV    "SD"

// TODO: erase sector 1 to 7, then write to sector 5
#  define FLASH_BLOCK_START_ADDRESS 0x20000

int mount_disk (void)
{
  /* raw disk i/o */
  uint64_t memory_size_mb;
  uint32_t block_count;
  uint32_t block_size;

  if (disk_access_init (DISK_PDRV) != 0)
    {
      printk ("ERROR: Storage init ERROR!");
      return -EFAULT;
    }

  if (disk_access_ioctl (DISK_PDRV, DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
    {
      printk ("ERROR: Unable to get sector count; ");
      return -EFAULT;
    }
  printk ("Block count %u; ", block_count);

  if (disk_access_ioctl (DISK_PDRV, DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
    {
      printk ("ERROR: Unable to get sector size; ");
      return -EFAULT;
    }
  printk ("Sector size %u\n", block_size);

  memory_size_mb = (uint64_t)block_count * block_size;
  printk ("Memory Size(MB) %u; ", (uint32_t) (memory_size_mb >> 20));

  mp.mnt_point = disk_mount_pt;

  int res = fs_mount (&mp);

  if (res == FR_OK)
    {
      printk ("Disk mounted; ");
      return 0;
    }
  else
    {
      printk ("Error mounting disk; ");
      return -EFAULT;
    }
}

bool __file_exist (const char *filename)
{
  struct fs_file_t file = {0};
  int rc = fs_open (&file, filename, FS_O_READ);
  if (rc < 0)
    {
      return false;
    }
  fs_close (&file);
  return true;
}
#endif /* ANIMULA_ZEPHYR */

#ifdef ANIMULA_LINUX
int mount_disk (void)
{
  os_printk ("int mount_disk (void)\n");
  return 0;
}
#endif /* ANIMULA_LINUX */
