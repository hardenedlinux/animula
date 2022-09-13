#ifndef __ANIMULA_GPIO_H__
#define __ANIMULA_GPIO_H__

/*  Copyright (C) 2019,2020
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

#ifdef ANIMULA_ZEPHYR
#  include "devicetree.h"
#  include <drivers/gpio.h>
#  include <drivers/i2c.h>

typedef enum
{
  SUPERDEVICE_TYPE_GPIO_PIN = 0,
  SUPERDEVICE_TYPE_I2C = 1,
  SUPERDEVICE_TYPE_SPI = 2,
} super_device_type_t;

typedef struct super_device_t
{
  const struct device *dev;
  super_device_type_t type;
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

#  define GPIO_PA9_NODE  DT_ALIAS (gpioa9)
#  define GPIO_PA9       DT_GPIO_LABEL (GPIO_PA9_NODE, gpios)
#  define GPIO_PA9_PIN   DT_GPIO_PIN (GPIO_PA9_NODE, gpios)
#  define GPIO_PA9_FLAGS DT_GPIO_FLAGS (GPIO_PA9_NODE, gpios)

#  define GPIO_PA10_NODE  DT_ALIAS (gpioa10)
#  define GPIO_PA10       DT_GPIO_LABEL (GPIO_PA10_NODE, gpios)
#  define GPIO_PA10_PIN   DT_GPIO_PIN (GPIO_PA10_NODE, gpios)
#  define GPIO_PA10_FLAGS DT_GPIO_FLAGS (GPIO_PA10_NODE, gpios)

#  define GPIO_PB4_NODE  DT_ALIAS (gpiob4)
#  define GPIO_PB4       DT_GPIO_LABEL (GPIO_PB4_NODE, gpios)
#  define GPIO_PB4_PIN   DT_GPIO_PIN (GPIO_PB4_NODE, gpios)
#  define GPIO_PB4_FLAGS DT_GPIO_FLAGS (GPIO_PB4_NODE, gpios)

#  define GPIO_PA8_NODE  DT_ALIAS (gpioa8)
#  define GPIO_PA8       DT_GPIO_LABEL (GPIO_PA8_NODE, gpios)
#  define GPIO_PA8_PIN   DT_GPIO_PIN (GPIO_PA8_NODE, gpios)
#  define GPIO_PA8_FLAGS DT_GPIO_FLAGS (GPIO_PA8_NODE, gpios)

#  define GPIO_PB5_NODE  DT_ALIAS (gpiob5)
#  define GPIO_PB5       DT_GPIO_LABEL (GPIO_PB5_NODE, gpios)
#  define GPIO_PB5_PIN   DT_GPIO_PIN (GPIO_PB5_NODE, gpios)
#  define GPIO_PB5_FLAGS DT_GPIO_FLAGS (GPIO_PB5_NODE, gpios)

#  define GPIO_PB6_NODE  DT_ALIAS (gpiob6)
#  define GPIO_PB6       DT_GPIO_LABEL (GPIO_PB6_NODE, gpios)
#  define GPIO_PB6_PIN   DT_GPIO_PIN (GPIO_PB6_NODE, gpios)
#  define GPIO_PB6_FLAGS DT_GPIO_FLAGS (GPIO_PB6_NODE, gpios)

#  define GPIO_PB7_NODE  DT_ALIAS (gpiob7)
#  define GPIO_PB7       DT_GPIO_LABEL (GPIO_PB7_NODE, gpios)
#  define GPIO_PB7_PIN   DT_GPIO_PIN (GPIO_PB7_NODE, gpios)
#  define GPIO_PB7_FLAGS DT_GPIO_FLAGS (GPIO_PB7_NODE, gpios)

#  define GPIO_PB8_NODE  DT_ALIAS (gpiob8)
#  define GPIO_PB8       DT_GPIO_LABEL (GPIO_PB8_NODE, gpios)
#  define GPIO_PB8_PIN   DT_GPIO_PIN (GPIO_PB8_NODE, gpios)
#  define GPIO_PB8_FLAGS DT_GPIO_FLAGS (GPIO_PB8_NODE, gpios)

#  define GPIO_PB9_NODE  DT_ALIAS (gpiob9)
#  define GPIO_PB9       DT_GPIO_LABEL (GPIO_PB9_NODE, gpios)
#  define GPIO_PB9_PIN   DT_GPIO_PIN (GPIO_PB9_NODE, gpios)
#  define GPIO_PB9_FLAGS DT_GPIO_FLAGS (GPIO_PB9_NODE, gpios)

#  define GPIO_PA2_NODE  DT_ALIAS (gpioa2)
#  define GPIO_PA2       DT_GPIO_LABEL (GPIO_PA2_NODE, gpios)
#  define GPIO_PA2_PIN   DT_GPIO_PIN (GPIO_PA2_NODE, gpios)
#  define GPIO_PA2_FLAGS DT_GPIO_FLAGS (GPIO_PA2_NODE, gpios)

#  define GPIO_PA3_NODE  DT_ALIAS (gpioa3)
#  define GPIO_PA3       DT_GPIO_LABEL (GPIO_PA3_NODE, gpios)
#  define GPIO_PA3_PIN   DT_GPIO_PIN (GPIO_PA3_NODE, gpios)
#  define GPIO_PA3_FLAGS DT_GPIO_FLAGS (GPIO_PA3_NODE, gpios)

#  define GPIO_PB3_NODE  DT_ALIAS (gpiob3)
#  define GPIO_PB3       DT_GPIO_LABEL (GPIO_PB3_NODE, gpios)
#  define GPIO_PB3_PIN   DT_GPIO_PIN (GPIO_PB3_NODE, gpios)
#  define GPIO_PB3_FLAGS DT_GPIO_FLAGS (GPIO_PB3_NODE, gpios)

#  define GPIO_PB10_NODE  DT_ALIAS (gpiob10)
#  define GPIO_PB10       DT_GPIO_LABEL (GPIO_PB10_NODE, gpios)
#  define GPIO_PB10_PIN   DT_GPIO_PIN (GPIO_PB10_NODE, gpios)
#  define GPIO_PB10_FLAGS DT_GPIO_FLAGS (GPIO_PB10_NODE, gpios)

#  define GPIO_PB15_NODE  DT_ALIAS (gpiob15)
#  define GPIO_PB15       DT_GPIO_LABEL (GPIO_PB15_NODE, gpios)
#  define GPIO_PB15_PIN   DT_GPIO_PIN (GPIO_PB15_NODE, gpios)
#  define GPIO_PB15_FLAGS DT_GPIO_FLAGS (GPIO_PB15_NODE, gpios)

#  define GPIO_PB14_NODE  DT_ALIAS (gpiob14)
#  define GPIO_PB14       DT_GPIO_LABEL (GPIO_PB14_NODE, gpios)
#  define GPIO_PB14_PIN   DT_GPIO_PIN (GPIO_PB14_NODE, gpios)
#  define GPIO_PB14_FLAGS DT_GPIO_FLAGS (GPIO_PB14_NODE, gpios)

#  define GPIO_PB13_NODE  DT_ALIAS (gpiob13)
#  define GPIO_PB13       DT_GPIO_LABEL (GPIO_PB13_NODE, gpios)
#  define GPIO_PB13_PIN   DT_GPIO_PIN (GPIO_PB13_NODE, gpios)
#  define GPIO_PB13_FLAGS DT_GPIO_FLAGS (GPIO_PB13_NODE, gpios)

#  define GPIO_PB12_NODE  DT_ALIAS (gpiob12)
#  define GPIO_PB12       DT_GPIO_LABEL (GPIO_PB12_NODE, gpios)
#  define GPIO_PB12_PIN   DT_GPIO_PIN (GPIO_PB12_NODE, gpios)
#  define GPIO_PB12_FLAGS DT_GPIO_FLAGS (GPIO_PB12_NODE, gpios)

#endif /* ANIMULA_ZEPHYR */

#endif /* __ANIMULA_GPIO_H__ */
