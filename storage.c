/*  Copyright (C) 2026 HardenedLinux Community
 *        Nala Ginrut <roy@hardenedlinux.org>
 *  Animula is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  Animula is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "storage.h"

#if defined ANIMULA_ZEPHYR
#  include <fcntl.h>
#  include <zephyr/device.h>
#  include <zephyr/devicetree.h>
#  include <zephyr/drivers/flash.h>
#  include <zephyr/fs/fs.h>
#  include <zephyr/kernel.h>

#define FLASH_NODE DT_CHOSEN(zephyr_flash)

static const struct device *flash_device = NULL;

static inline void zephyr_flash_init(void)
{
  flash_device = DEVICE_DT_GET(FLASH_NODE);

  if (device_is_ready(flash_device))
    {
      os_printk("Found flash device %s.\n",
                DEVICE_DT_NAME(FLASH_NODE));
      os_printk("Flash I/O commands can be run.\n");
    }
  else
    {
      os_printk("**No flash device found!**\n");
      os_printk("Run set_device <name> to specify one "
                "before using other commands.\n");
    }
}

static inline int zephyr_flash_erase(size_t offset, size_t size)
{
  int ret = flash_erase(flash_device, offset, size);

  if (0 != ret)
    {
      os_printk("Flash error: flash_erase failed - %d\n", ret);
      return ret;
    }

  return ret;
}

static inline int zephyr_flash_write(const char *buf, size_t offset,
                                     size_t size)
{
  int ret = flash_write(flash_device, offset, buf, size);

  if (0 != ret)
    {
      os_printk("Flash error: flash_write failed - %d\n", ret);
      return ret;
    }

  return ret;
}

static inline int zephyr_flash_read(char *buf, size_t offset, size_t size)
{
  int ret = flash_read(flash_device, offset, buf, size);

  if (0 != ret)
    os_printk("Flash error: flash_read error - %d\n", ret);

  return ret;
}
#endif

void os_flash_init(void)
{
#if defined ANIMULA_ZEPHYR
  zephyr_flash_init();
#endif
}

int os_flash_erase(size_t offset, size_t size)
{
  int ret = 0;

#if defined ANIMULA_ZEPHYR
  ret = zephyr_flash_erase(offset, size);
#endif

  if (0 != ret)
    os_printk("os_flash_erase: error\n");

  return ret;
}

int os_flash_write(const char *buf, size_t offset, size_t size)
{
  int ret = 0;

#if defined ANIMULA_ZEPHYR
  ret = zephyr_flash_write(buf, offset, size);
#endif

  if (0 != ret)
    os_printk("os_flash_write: error\n");

  return ret;
}

int os_flash_read(char *buf, size_t offset, size_t size)
{
  int ret = 0;

#if defined ANIMULA_ZEPHYR
  ret = zephyr_flash_read(buf, offset, size);
#endif

  if (0 != ret)
    os_printk("os_flash_read: error\n");

  return ret;
}

int os_open(const char *path, int flags)
{
  /* Only read-only open is exercised today. */
  if (OS_O_RDONLY != flags)
    return -1;

#if defined ANIMULA_LINUX
  return open(path, O_RDONLY);
#elif defined ANIMULA_ZEPHYR
  return zephyr_open(path, FS_O_READ);
#else
  return -1;
#endif
}

int os_close(int fd)
{
#if defined ANIMULA_LINUX
  return close(fd);
#elif defined ANIMULA_ZEPHYR
  return zephyr_close(fd);
#else
  return -1;
#endif
}

int os_file_exist(const char *path)
{
#if defined ANIMULA_LINUX
  struct stat st = {0};
  return (stat(path, &st) == 0);
#elif defined ANIMULA_ZEPHYR
  struct fs_dirent entry = {0};
  return (zephyr_stat(path, &entry) == 0);
#else
  return 0;
#endif
}

int os_open_input_file(const char *filename)
{
  int fd = os_open(filename, OS_O_RDONLY);

  if (fd < 0)
    {
      os_printk("Open file \"%s\" failed!\n", filename);
      os_abort(-1);
    }

  return fd;
}

int os_read(int fd, void *buf, size_t count)
{
  int ret = -1;

#if defined ANIMULA_LINUX
  ret = read(fd, buf, count);
#elif defined ANIMULA_ZEPHYR
  ret = zephyr_read(fd, buf, count);
#endif

  if (ret < 0)
    {
      os_printk("Read file \"%d\" failed!\n", fd);
      os_abort(-1);
    }

  return ret;
}

void os_read_u32(int fd, void *buf)
{
#if defined ANIMULA_BIG_ENDIAN
  os_read(fd, buf, 1);
  os_read(fd, buf + 1, 1);
  os_read(fd, buf + 2, 1);
  os_read(fd, buf + 3, 1);
#else
  os_read(fd, buf + 3, 1);
  os_read(fd, buf + 2, 1);
  os_read(fd, buf + 1, 1);
  os_read(fd, buf, 1);
#endif
}

void os_read_u16(int fd, void *buf)
{
#if defined ANIMULA_BIG_ENDIAN
  os_read(fd, buf, 1);
  os_read(fd, buf + 1, 1);
#else
  os_read(fd, buf + 1, 1);
  os_read(fd, buf, 1);
#endif
}
