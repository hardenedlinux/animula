/*  Copyright (C) 2020-2021
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

#include "primitives.h"
#ifdef LAMBDACHIP_ZEPHYR
// #  include <string.h> // memncpy
#  include <drivers/gpio.h>
#  include <vos/drivers/gpio.h>
#endif /* LAMBDACHIP_ZEPHYR */

GLOBAL_DEF (prim_t, prim_table[PRIM_MAX]) = {0};

// primitives implementation

static inline imm_int_t _int_add (imm_int_t x, imm_int_t y)
{
  // os_printk ("%d + %d = %d\n", x, y, x + y);
  return x + y;
}

static inline imm_int_t _int_sub (imm_int_t x, imm_int_t y)
{
  /* os_printk ("%d - %d = %d\n", x, y, x - y); */
  return x - y;
}

static inline imm_int_t _int_mul (imm_int_t x, imm_int_t y)
{
  return x * y;
}

imm_int_t _int_modulo (imm_int_t x, imm_int_t y)
{
  imm_int_t m = x % y;
  return y < 0 ? (m <= 0 ? m : m + y) : (m >= 0 ? m : m + y);
}

static inline imm_int_t _int_remainder (imm_int_t x, imm_int_t y)
{
  return x % y;
}

void _object_print (object_t obj)
{
  object_printer (obj);
}

static bool _int_eq (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  // os_printk ("%p == %p is %d\n", x->value, y->value, x->value == y->value);
  return (imm_int_t)x->value == (imm_int_t)y->value;
}

static bool _int_lt (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value < (imm_int_t)y->value;
}

static bool _int_gt (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value > (imm_int_t)y->value;
}

static bool _int_le (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value <= (imm_int_t)y->value;
}

static bool _int_ge (object_t x, object_t y)
{
  VALIDATE (x, imm_int);
  VALIDATE (y, imm_int);
  return (imm_int_t)x->value >= (imm_int_t)y->value;
}
// --------------------------------------------------

static bool _not (object_t obj)
{
  return is_false (obj);
}

static bool _eq (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;

  if (t1 != t2)
    {
      return false;
    }
  else if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
    {
      return true;
    }
  else if (procedure == t1)
    {
      return (a->proc.entry == b->proc.entry);
    }

  return (a->value == b->value);
}

static bool _eqv (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
      /* case record: */
      /* case bytevector: */
      /* case float: */
    case imm_int:
      {
        ret = _int_eq (a, b);
        break;
      }
    case symbol:
      {
        ret = symbol_eq (a, b);
        break;
      }
    case boolean:
      {
        ret = (a == b);
        break;
      }
    case pair:
    case string:
    case vector:
    case list:
      {
        if (list == t1 && (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b)))
          {
            ret = true;
          }
        else
          {
            /* NOTE:
             * For collections, eqv? only compare their head pointer.
             */
            ret = (a->value == b->value);
          }
        break;
      }
    case procedure:
      {
        /* NOTE:
         * R7RS demands the procedure that has side-effects for
         * different behaviour or return values to be the different
         * object. This is guaranteed by the compiler, not by the VM.
         */
        ret = (a->proc.entry == b->proc.entry);
        break;
      }
    default:
      {
        os_printk ("eqv?: The type %d hasn't implemented yet\n", t1);
        panic ("");
      }
      // TODO-1
      /* case character: */
      /*   { */
      /*     ret = character_eq (a, b); */
      /*     break; */
      /*   } */

      // TODO-2
      // Inexact numbers
    }

  return ret;
}

static bool _equal (object_t a, object_t b)
{
  otype_t t1 = a->attr.type;
  otype_t t2 = b->attr.type;
  bool ret = false;

  if (t1 != t2)
    return false;

  switch (t1)
    {
    case pair:
      {
        pair_t ap = (pair_t)a->value;
        pair_t bp = (pair_t)b->value;
        ret = (_equal (ap->car, bp->car) && _equal (ap->cdr, bp->cdr));
        break;
      }
    case string:
      {
        ret = str_eq (a, b);
        break;
      }
    case list:
      {
        if (LIST_IS_EMPTY (a) && LIST_IS_EMPTY (b))
          {
            ret = true;
          }
        else
          {
            obj_list_head_t *h1 = LIST_OBJECT_HEAD (a);
            obj_list_head_t *h2 = LIST_OBJECT_HEAD (b);
            obj_list_t n1 = NULL;
            obj_list_t n2 = SLIST_FIRST (h2);

            SLIST_FOREACH (n1, h1, next)
            {
              ret = _equal (n1->obj, n2->obj);
              if (!ret)
                break;
              n2 = SLIST_NEXT (n2, next);
            }
          }
        break;
      }
    case vector:
      {
        panic ("equal?: vector hasn't been implemented yet!\n");
        /* TODO: iterate each element and call equal? */
        break;
      }
      /* case bytevector: */
      /*   { */
      /*     ret = bytevector_eq (a, b); */
      /*     break; */
      /*   } */
    default:
      {
        ret = _eqv (a, b);
        break;
      }
    }

  return ret;
}

static imm_int_t _os_usleep (object_t us)
{
  // zephyr/include/ kernel.h
  // 761: * this must be understood before attempting to use k_usleep(). Use
  // with 769:__syscall int32_t k_usleep(int32_t us);

  // linux
  // int usleep(useconds_t usec);
  return os_usleep ((int32_t)us->value);
}

#ifdef LAMBDACHIP_ZEPHYR

extern GLOBAL_DEF (super_device, super_dev_led0);
extern GLOBAL_DEF (super_device, super_dev_led1);
extern GLOBAL_DEF (super_device, super_dev_led2);
extern GLOBAL_DEF (super_device, super_dev_led3);

extern GLOBAL_DEF (super_device, super_dev_gpio_pa9);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa10);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb4);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa8);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb5);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb6);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb7);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb8);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb9);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa2);
extern GLOBAL_DEF (super_device, super_dev_gpio_pa3);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb3);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb10);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb15);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb14);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb13);
extern GLOBAL_DEF (super_device, super_dev_gpio_pb12);

extern GLOBAL_DEF (super_device, super_dev_i2c2);
extern GLOBAL_DEF (super_device, super_dev_i2c3);

static object_t _os_get_board_id (void)
{
  static uint32_t g_board_uid[3] = {0, 0, 0};
  // copy 96 bit UID as 3 uint32_t integer
  // then convert to 24 bytes of string
  if (0 == g_board_uid[0])
    {
      os_memcpy (g_board_uid, (char *)UID_BASE, sizeof (g_board_uid));
    }
  char uid[25] = {0};
  // snprintk, last is \0, shall be included
  os_snprintk (uid, 25, "%08X%08X%08X", g_board_uid[0], g_board_uid[1],
               g_board_uid[2]);
  return create_new_string (uid);
}

extern const struct device *GLOBAL_REF (dev_led0);
extern const struct device *GLOBAL_REF (dev_led1);
extern const struct device *GLOBAL_REF (dev_led2);
extern const struct device *GLOBAL_REF (dev_led3);

static super_device *translate_supper_dev_from_symbol (object_t sym)
{
  super_device *ret = NULL;
  static const char char_dev_led0[] = "dev_led0";
  static const char char_dev_led1[] = "dev_led1";
  static const char char_dev_led2[] = "dev_led2";
  static const char char_dev_led3[] = "dev_led3";
  static const char char_gpio_pa9[] = "dev_gpio_pa9";
  static const char char_gpio_pa10[] = "dev_gpio_pa10";
  static const char char_gpio_pb4[] = "dev_gpio_pb4";
  static const char char_gpio_pa8[] = "dev_gpio_pa8";
  static const char char_gpio_pb5[] = "dev_gpio_pb5";
  static const char char_gpio_pb6[] = "dev_gpio_pb6";
  static const char char_gpio_pb7[] = "dev_gpio_pb7";
  static const char char_gpio_pb8[] = "dev_gpio_pb8";
  static const char char_gpio_pb9[] = "dev_gpio_pb9";
  static const char char_gpio_pa2[] = "dev_gpio_pa2";
  static const char char_gpio_pa3[] = "dev_gpio_pa3";
  static const char char_gpio_pb3[] = "dev_gpio_pb3";
  static const char char_gpio_pb10[] = "dev_gpio_pb10";
  static const char char_gpio_pb15[] = "dev_gpio_pb15";
  static const char char_gpio_pb14[] = "dev_gpio_pb14";
  static const char char_gpio_pb13[] = "dev_gpio_pb13";
  static const char char_gpio_pb12[] = "dev_gpio_pb12";
  static const char char_i2c2[] = "dev_i2c2";
  static const char char_i2c3[] = "dev_i2c3";

  const char *str_buf = GET_SYMBOL ((u32_t)sym->value);
  size_t len = os_strnlen (str_buf, MAX_STR_LEN);
  if (0 == os_strncmp (str_buf, char_dev_led0, len))
    {
      ret = &(GLOBAL_REF (super_dev_led0));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led1, len))
    {
      ret = &(GLOBAL_REF (super_dev_led1));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led2, len))
    {
      ret = &(GLOBAL_REF (super_dev_led2));
    }
  else if (0 == os_strncmp (str_buf, char_dev_led3, len))
    {
      ret = &(GLOBAL_REF (super_dev_led3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa9, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa9));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa10, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa10));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb4, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb4));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa8, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa8));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb5, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb5));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb6, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb6));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb7, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb7));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb8, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb8));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb9, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb9));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa2, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa2));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pa3, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pa3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb3, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb3));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb10, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb10));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb15, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb15));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb14, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb14));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb13, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb13));
    }
  else if (0 == os_strncmp (str_buf, char_gpio_pb12, len))
    {
      ret = &(GLOBAL_REF (super_dev_gpio_pb12));
    }
  else if (0 == os_strncmp (str_buf, char_i2c2, len))
    {
      ret = &(GLOBAL_REF (super_dev_i2c2));
    }
  else if (0 == os_strncmp (str_buf, char_i2c3, len))
    {
      ret = &(GLOBAL_REF (super_dev_i2c3));
    }
  else
    {
      os_printk ("BUG: Invalid dev_led name %s!\n", str_buf);
      panic ("");
    }

  return ret;
}

static object_t _os_device_configure (object_t ret, object_t obj)
{
  // const char *str_buf = GET_SYBOL ((u32_t)obj->value);
  super_device *p = translate_supper_dev_from_symbol (obj);

  if (p->type == SUPERDEVICE_TYPE_GPIO_PIN)
    {
      gpio_pin_configure (p->dev, p->gpio_pin, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    }
  else if (p->type == SUPERDEVICE_TYPE_I2C)
    {
      i2c_configure (p->dev, 0);
    }
  else if (p->type == SUPERDEVICE_TYPE_SPI)
    {
    }
  else
    {
      assert (-1 && "device type not defined, cannot handle");
    }

  // os_printk ("imm_int_t _os_gpio_toggle (%s)\n", str_buf);
  ret->value = (void *)0;
  return ret;
}

// dev->value is the string/symbol refer to of a super_device
static object_t _os_gpio_set (object_t ret, object_t obj, object_t v)
{
  super_device *p = translate_supper_dev_from_symbol (obj);
  ret->value = (void *)gpio_pin_set (p->dev, p->gpio_pin, (int)v->value);
  return ret;
}

static object_t _os_gpio_toggle (object_t ret, object_t obj)
{
  super_device *p = translate_supper_dev_from_symbol (obj);
  ret->value = (void *)gpio_pin_toggle (p->dev, p->gpio_pin);
  return ret;
}

/* LAMBDACHIP_ZEPHYR */
#elif defined LAMBDACHIP_LINUX
static object_t _os_get_board_id (void)
{
  // static char[] board_id = "GNU/Linux";
  // return board_id;
  // object_t obj;
  // Object obj = {.attr = {.type = mut_string, .gc = 0}, .value = 0};
  // obj = {.attr = {.type = } };
  // FIXME: return create a mut_string and return
  return (object_t)NULL;
}

static object_t _os_device_configure (object_t ret, object_t dev)
{
  const char *str_buf = GET_SYMBOL ((u32_t)dev->value);
  os_printk ("object_t _os_device_configure (%s)\n", str_buf);
  ret->value = (void *)0;
  return ret;
}

static object_t _os_gpio_set (object_t ret, object_t dev, object_t v)
{
  const char *str_buf = GET_SYMBOL ((u32_t)dev->value);
  os_printk ("object_t _os_gpio_set (%s, %d)\n", str_buf, (imm_int_t)v->value);
  ret->value = (void *)0;
  return ret;
}

static object_t _os_gpio_toggle (object_t ret, object_t obj)
{
  const char *str_buf = GET_SYMBOL ((u32_t)obj->value);
  os_printk ("object_t _os_gpio_toggle (%s)\n", str_buf);
  ret->value = (void *)0;
  return ret;
}
#endif /* LAMBDACHIP_LINUX */

void primitives_init (void)
{
  /* NOTE: If fn is NULL, then it's inlined to call_prim
   */
  def_prim (0, "ret", 0, NULL);
  def_prim (1, "pop", 0, NULL);
  def_prim (2, "add", 2, (void *)_int_add);
  def_prim (3, "sub", 2, (void *)_int_sub);
  def_prim (4, "mul", 2, (void *)_int_mul);
  def_prim (5, "fract_div", 2, NULL);
  def_prim (6, "object_print", 1, (void *)_object_print);
  def_prim (7, "apply", 2, NULL);
  def_prim (8, "not", 1, (void *)_not);
  def_prim (9, "int_eq", 2, (void *)_int_eq);
  def_prim (10, "int_lt", 2, (void *)_int_lt);
  def_prim (11, "int_gt", 2, (void *)_int_gt);
  def_prim (12, "int_le", 2, (void *)_int_le);
  def_prim (13, "int_ge", 2, (void *)_int_ge);
  def_prim (14, "restore", 0, NULL);
  def_prim (15, "reserved_1", 0, NULL);

  // extended primitives
  def_prim (16, "modulo", 2, (void *)_int_modulo);
  def_prim (17, "remainder", 2, (void *)_int_remainder);
  def_prim (18, "foreach", 2, NULL);
  def_prim (19, "map", 2, NULL);
  def_prim (20, "list_ref", 2, (void *)list_ref);
  def_prim (21, "list_set", 3, (void *)list_set);
  def_prim (22, "list_append", 2, (void *)list_append);
  def_prim (23, "eq", 2, (void *)_eq);
  def_prim (24, "eqv", 2, (void *)_eqv);
  def_prim (25, "equal", 2, (void *)_equal);
  def_prim (26, "prim_usleep", 1, (void *)_os_usleep);
  // #ifdef LAMBDACHIP_ZEPHYR
  def_prim (27, "device_configure", 2, (void *)_os_device_configure);
  def_prim (28, "gpio_set", 2, (void *)_os_gpio_set);
  def_prim (29, "gpio_toggle", 1, (void *)_os_gpio_toggle);
  def_prim (30, "get_board_id", 2, (void *)_os_get_board_id);
  // // gpio_pin_set(dev_led0, LED0_PIN, (((cnt) % 5) == 0) ? 1 : 0);

  // #endif /* LAMBDACHIP_ZEPHYR */
}

char *prim_name (u16_t pn)
{
#if defined LAMBDACHIP_DEBUG
  if (pn >= PRIM_MAX)
    {
      VM_DEBUG ("Invalid prim number: %d\n", pn);
      panic ("prim_name halt\n");
    }

  return GLOBAL_REF (prim_table)[pn]->name;
#else
  return "";
#endif
}

prim_t get_prim (u16_t pn)
{
  return GLOBAL_REF (prim_table)[pn];
}

void primitives_clean (void)
{
  for (int i = 0; i <= object_print; i++)
    {
      os_free (GLOBAL_REF (prim_table)[i]);
      GLOBAL_REF (prim_table)[i] = NULL;
    }
}
