#ifndef __LAMBDACHIP_GPIO_H__
#define __LAMBDACHIP_GPIO_H__

/*  Copyright (C) 2019,2020
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

#ifdef LAMBDACHIP_ZEPHYR
#  include "devicetree.h"
#  include <drivers/gpio.h>
#  include <drivers/i2c.h>

typedef struct super_device_t
{
  const struct device *dev;
  int gpio_pin;
} super_device;

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

#endif /* __LAMBDACHIP_GPIO_H__ */
