/*
 *  Copyright (C) 2020-2021
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *        Rafael Lee <rafaellee.img@gmail.com>
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
 *
 */

#include "os.h"
#include "debug.h"

#if defined LAMBDACHIP_LINUX
//
#elif defined LAMBDACHIP_ZEPHYR
#  include <fs/fs.h>           // fs_open
#  include <fs/fs_interface.h> // fs_file_t
GLOBAL_DEF (struct fs_file_t,
            file_descriptors[CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR])
  = {0};

#  ifndef CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR
#    error \
      "Please specify CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR in prj.conf"
#  else

int zephyr_open (const char *pathname, int flags)
{
  int fd = 0;
  // 0: stdout
  // 1: stdin
  // 2: stderr
  struct fs_file_t *file = NULL;
  for (size_t i = 3; i < CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR; i++)
    {
      if (NULL == (GLOBAL_REF (file_descriptors)[i]).filep)
        {
          file = &(GLOBAL_REF (file_descriptors)[i]);
          fd = i;
          break;
        }
    }
  int ret = fs_open (file, pathname, flags);
  // file is modified in fs_open
  if (ret < 0) // open error
    {
      return ret;
    }
  else if (0 == ret) // success
    {
      return fd;
    }
  else // ret > 0
    {
      assert (0 && "Error, zephyr fs_open shall not return a positive value");
      return ret;
    }
}

int zephyr_close (int fd)
{
  struct fs_file_t *file = &(GLOBAL_REF (file_descriptors))[fd];
  int ret = fs_close (file);
  if (0 == ret) // success
    {
      (GLOBAL_REF (file_descriptors))[fd].filep = (void *)NULL;
      memset ((void *)file, 0, sizeof (struct fs_file_t));
      // file->filep = (void*)NULL;
      return 0;
    }
  else // failed
    {
      return ret;
    }
}

ssize_t zephyr_read (int fd, void *buf, size_t count)
{
  struct fs_file_t file = (GLOBAL_REF (file_descriptors))[fd];
  ssize_t size = 0;
  size = fs_read (&file, buf, count);
  return size;
}

int zephyr_stat (const char *path)
{
  struct fs_dirent entry = {0};
  return fs_stat (path, &entry);
}

#  endif /* CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR */
#endif   /* LAMBDACHIP_ZEPHYR */
