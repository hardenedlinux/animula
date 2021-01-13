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

#ifdef LAMBDACHIP_ZEPHYR
#  include "devicetree.h"
#  include <drivers/gpio.h>
#  include <drivers/i2c.h>

#  define LED0_NODE       DT_ALIAS (led0)
#  define LED1_NODE       DT_ALIAS (led1)
#  define LED2_NODE       DT_ALIAS (led2)
#  define LED3_NODE       DT_ALIAS (led3)
#  define BLEDISABLE_NODE DT_ALIAS (bledisable)
#  define SW0_NODE        DT_ALIAS (sw0)

#  if DT_NODE_HAS_STATUS(LED0_NODE, okay)

#    define LED0       DT_GPIO_LABEL (LED0_NODE, gpios)
#    define LED0_PIN   DT_GPIO_PIN (LED0_NODE, gpios)
#    define LED0_FLAGS DT_GPIO_FLAGS (LED0_NODE, gpios)

#    define LED1       DT_GPIO_LABEL (LED1_NODE, gpios)
#    define LED1_PIN   DT_GPIO_PIN (LED1_NODE, gpios)
#    define LED1_FLAGS DT_GPIO_FLAGS (LED1_NODE, gpios)

#    define LED2       DT_GPIO_LABEL (LED2_NODE, gpios)
#    define LED2_PIN   DT_GPIO_PIN (LED2_NODE, gpios)
#    define LED2_FLAGS DT_GPIO_FLAGS (LED2_NODE, gpios)

#    define LED3       DT_GPIO_LABEL (LED3_NODE, gpios)
#    define LED3_PIN   DT_GPIO_PIN (LED3_NODE, gpios)
#    define LED3_FLAGS DT_GPIO_FLAGS (LED3_NODE, gpios)

#    define BLEDISABLE       DT_GPIO_LABEL (BLEDISABLE_NODE, gpios)
#    define BLEDISABLE_PIN   DT_GPIO_PIN (BLEDISABLE_NODE, gpios)
#    define BLEDISABLE_FLAGS DT_GPIO_FLAGS (BLEDISABLE_NODE, gpios)

#    define SW0       DT_GPIO_LABEL (SW0_NODE, gpios)
#    define SW0_PIN   DT_GPIO_PIN (SW0_NODE, gpios)
#    define SW0_FLAGS DT_GPIO_FLAGS (SW0_NODE, gpios)

#    define I2C2_LABEL DT_LABEL (DT_NODELABEL (i2c2))
#    define I2C3_LABEL DT_LABEL (DT_NODELABEL (i2c3))
#  endif /* DT_NODE_HAS_STATUS */
#endif   /* LAMBDACHIP_ZEPHYR */

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

  gpio_pin_set (dev_led0, LED0_PIN, 0);
  gpio_pin_set (dev_led1, LED1_PIN, 0);
  gpio_pin_set (dev_led2, LED2_PIN, 0);
  gpio_pin_set (dev_led3, LED3_PIN, 0);

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
  if (!strncmp (*lef->sig, "LEF", 3))
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
