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

#include "lambdachip.h"
#include "dts_bindings.h"

vm_t lambdachip_init (void)
{
  vm_t vm = (vm_t)os_malloc (sizeof (struct LambdaVM));

#ifndef LAMBDACHIP_LINUX
  os_flash_init ();
#endif

  init_ram_heap ();
  primitives_init ();
  // NOTE: The allocated vm object will never be freed.
  vm_init (vm);
  stdio_init ();
  return vm;
}

void lambdachip_clean (vm_t vm)
{
  vm_clean (vm);
  primitives_clean ();
  stdio_clean ();
}

#ifdef LAMBDACHIP_ZEPHYR
const struct device *dev_led0;
const struct device *dev_led1;
const struct device *dev_led2;
const struct device *dev_led3;
#endif /* LAMBDACHIP_ZEPHYR */

void lambdachip_start (void)
{
  vm_t vm = lambdachip_init ();

  // TODO: Print lambdachip version information
  os_printk ("Welcome to Lambdachip! %s\n", get_platform_info ());
  os_printk ("Author: Mu Lei known as Nala Ginrut <mulei@gnu.org>\n");
  os_printk ("Type `help' to get help\n");

#ifndef LAMBDACHIP_LINUX
  dev_led0 = device_get_binding (LED0);
  dev_led1 = device_get_binding (LED1);
  dev_led2 = device_get_binding (LED2);
  dev_led3 = device_get_binding (LED3);

  int ret = 0;
  /* Set LED pin as output */
  ret
    = gpio_pin_configure (dev_led0, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
  if (ret < 0)
    {
      return;
    }
  ret
    = gpio_pin_configure (dev_led1, LED1_PIN, GPIO_OUTPUT_ACTIVE | LED1_FLAGS);
  if (ret < 0)
    {
      return;
    }
  ret
    = gpio_pin_configure (dev_led2, LED2_PIN, GPIO_OUTPUT_ACTIVE | LED2_FLAGS);
  if (ret < 0)
    {
      return;
    }
  ret
    = gpio_pin_configure (dev_led3, LED3_PIN, GPIO_OUTPUT_ACTIVE | LED3_FLAGS);
  if (ret < 0)
    {
      return;
    }

  gpio_pin_set (dev_led0, LED0_PIN, 1);
  gpio_pin_set (dev_led1, LED1_PIN, 1);
  gpio_pin_set (dev_led2, LED2_PIN, 1);
  gpio_pin_set (dev_led3, LED3_PIN, 1);

  // #ifdef LAMBDACHIP_LINUX
  //       struct fs_file_t file;
  lef_t lef;
  do
    {
      if (0 != mount_disk ())
        {
          break;
        }
      static const char fname[] = "/SD:/program.lef";
      //   load_lef_from_file_system
      // #endif

      u8_t buf[512];
      // struct fs_file_t file;
      int fd = open ("/SD:/calendar.txt", FS_O_READ);
      if (fd < 0)
        {
          break;
        }

      read (fd, buf, 512);
      os_printk ("/SD:/calendar.txt =\n");
      os_printk ("%s", buf);
      close (fd);

      lef = load_lef_from_file (fname);
    }
  while (0);
  if (!strncmp (lef->sig, "LEF", 3))
    {
      vm_load_lef (vm, lef);
      vm_run (vm);
      lambdachip_clean (vm);
    }
  else
    {
      run_shell (vm);
    }

#endif /* LAMBDACHIP_LINUX */
}
