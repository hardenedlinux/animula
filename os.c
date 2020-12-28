/**
 ******************************************************************************
 * @file    os.h
 * @author  Rafael Lee
 * @version v0.0.1
 * @date    2020 Dec 28th
 * @brief   os.c module
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/**
 * @brief
 * @param  None
 * @retval None
 */
/* ---------------------------------------------------------------------------*/

/**
 * \par Function
 *   function
 * \par Description
 *   description
 * \par Output
 *   None
 * \par return
 *   ret
 * \par Others
 *    multi-thread unsafe
 *    side effect global variable file_descriptors is modified
 */
// int open(const char *pathname, int flags);
// int open()
#ifdef LAMBDACHIP_ZEPHYR
#  include "os.h"
#  include <fs/fs.h>           // fs_open
#  include <fs/fs_interface.h> // fs_file_t
GLOBAL_DEF (struct fs_file_t,
            file_descriptors[CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR])
  = {0};

#  ifndef CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR
#    error \
      "Please specify CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR in prj.conf"
#  else

int open (const char *pathname, int flags)
{
  int fd = 0;
  // 0: stdout
  // 1: stdin
  // 2: stderr
  struct fs_file_t *file;
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

  // filep

  // prim_table[]{};
  // int fs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags);
  // zephyr/include/fs/fs.h
  // static char g_file_descriptors[]
}

/**
 * \par Function
 *   function
 * \par Description
 *   description
 * \par Output
 *   None
 * \par return
 *   ret
 * \par Others
 *    multi-thread unsafe
 *    side effect global variable file_descriptors is modified
 */
int close (int fd)
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

/**
 * \par Function
 *   function
 * \par Description
 *   Let fs_read handle error
 * \par Output
 *   None
 * \par return
 *   ret
 * \par Others
 *    multi-thread unsafe
 */
ssize_t read (int fd, void *buf, size_t count)
{
  struct fs_file_t file = (GLOBAL_REF (file_descriptors))[fd];
  ssize_t size = 0;
  size = fs_read (&file, buf, count);
  return size;
}

#  endif /* CONFIG_MAXIMUM_NUMBER_OPEN_FILE_DESCRIPTOR */
#endif   /* LAMBDACHIP_ZEPHYR */
/************************ (C) COPYRIGHT ************************END OF FILE****/
