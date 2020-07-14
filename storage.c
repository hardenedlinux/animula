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

#include "storage.h"

#if defined LAMBDACHIP_ZEPHYR
static struct device *flash_device = NULL;

static inline void zephyr_flash_init(void)
{
  flash_device = device_get_binding(DT_FLASH_DEV_NAME);

  if(flash_device)
    {
      os_printk("Found flash device %s.\n", DT_FLASH_DEV_NAME);
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
  int ret = flash_write_protection_set(flash_device, false);

  if(ret)
    {
      os_printk("Flash error: failed to disable flash protection - %d\n", ret);
      return ret;
    }

  ret = flash_erase(flash_device, offset, size);
  if(ret)
    {
      os_printk("Flash error: flash_erase failed - %d\n", ret);
      return ret;
    }

  ret = flash_write_protection_set(flash_device, true);
  if(ret)
    {
      os_printk("Flash error: failed to enable flash protection - %d\n", ret);
      return ret;
    }

  return ret;
}

static inline int zephyr_flash_write(const char* buf, size_t offset, size_t size)
{
  int ret = flash_write_protection_set(flash_device, false);

  if(ret)
    {
      os_printk("Flash error: failed to disable flash protection - %d\n", ret);
      return ret;
    }

  ret = flash_write(flash_device, offset, buf, size);
  if(ret)
    {
      os_printk("Flash error: flash_write failed - %d\n", ret);
      return ret;
    }

  ret = flash_write_protection_set(flash_device, true);
  if(ret)
    {
      os_printk("Flash error: failed to enable flash protection - %d\n", ret);
      return ret;
    }

  return ret;
}

static inline int zephyr_flash_read(char* buf, size_t offset, size_t size)
{

  int ret = flash_read(flash_device, offset, buf, size);
  if(ret)
    os_printk("Flash error: flash_read error - %d\n", ret);

  return ret;
}
#endif

void os_flash_init(void)
{
#if defined LAMBDACHIP_ZEPHYR
  zephyr_flash_init();
#endif
}

int os_flash_erase(size_t offset, size_t size)
{
  int ret = 0;

#if defined LAMBDACHIP_ZEPHYR
  ret = zephyr_flash_erase(offset, size);
#endif

  if(ret)
    os_printk("os_flash_erase: error\n");

  return ret;
}

int os_flash_write(const char* buf, size_t offset, size_t size)
{
  int ret = 0;

#if defined LAMBDACHIP_ZEPHYR
  ret = zephyr_flash_write(buf, offset, size);
#endif

  if(ret)
    os_printk("os_flash_write: error\n");

  return ret;
}

int os_flash_read(char* buf, size_t offset, size_t size)
{
  int ret = 0;

#if defined LAMBDACHIP_ZEPHYR
  ret = zephyr_flash_read(buf, offset, size);
#endif

  if(ret)
    os_printk("os_flash_read: error\n");

  return ret;
}

int os_open_input_file(const char *filename)
{
  int fd = -1;
#if defined LAMBDACHIP_LINUX
  if ((fd = open(filename, O_RDONLY)) < 0)
    {
      os_printk("Open file \"%s\" failed!\n", filename);
      exit(-1);
    }
#else
  os_printk("The current platform %s doesn't support filesystem!\n",
            get_platform_info());
  exit(-1);
#endif
  return fd;
}

int os_read(int fd, void *buf, size_t count)
{
  int ret = -1;
#if defined LAMBDACHIP_LINUX
  if ((ret = read(fd, buf, count)) < 0)
    {
      os_printk("Read file \"%d\" failed!\n", fd);
      exit(-1);
    }
#else
  os_printk("The current platform %s doesn't support filesystem!\n",
            get_platform_info());
  exit(-1);
#endif
  return ret;
}

void os_read_u32(int fd, void *buf)
{
#if defined LAMBDACHIP_BIG_ENDIAN
  os_read(fd, buf, 1);
  os_read(fd, buf+1, 1);
  os_read(fd, buf+2, 1);
  os_read(fd, buf+3, 1);
#else
  os_read(fd, buf+3, 1);
  os_read(fd, buf+2, 1);
  os_read(fd, buf+1, 1);
  os_read(fd, buf, 1);
#endif
}

void os_read_u16(int fd, void *buf)
{
#if defined LAMBDACHIP_BIG_ENDIAN
  os_read(fd, buf, 1);
  os_read(fd, buf+1, 1);
#else
  os_read(fd, buf+1, 1);
  os_read(fd, buf, 1);
#endif
}
