#ifndef __FILE_OPERATION_H__
#  define __FILE_OPERATION_H__

/*
 *  Copyright (C) 2020-2021
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

 *
 ******************************************************************************
 * @file
 * @author  Rafael Lee
 * @version v0.0.1
 * @date
 * @brief   Header for xxx.c module
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#  include <ff.h> // FATFS
#  include <fs/fs.h>

/* Exported types ------------------------------------------------------------*/
typedef struct flash_sector
{
  uint32_t offset;
  uint32_t size;
} flash_sector;

static FATFS fat_fs;

static struct fs_mount_t mp = {
  .type = FS_FATFS,
  .fs_data = &fat_fs,
};
/* Exported constants --------------------------------------------------------*/
static const char *disk_mount_pt = "/SD:";

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
int mount_disk (void);
bool fs_exist (struct fs_file_t file, const char *filename);

#endif /* __FILE_OPERATION_H__ */

/************************ (C) COPYRIGHT ************************END OF FILE****/
