/*  Copyright (C) 2020-2026
 *        "Mu Lei" known as "NalaGinrut" <NalaGinrut@gmail.com>
 *        "Rafael Lee"                   <rafaellee.img@gmail.com>
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

#include "number.h"

typedef float (*real_op_t) (float);

static object_t op_dispatch (vm_t vm, object_t ret, immu_object_t x,
                             real_op_t real_op)
{
  switch (x->attr.type)
    {
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        f.f = real_op (f.f);
        ret->value = (void *)f.v;
        ret->attr.type = real;
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        real_t f;
        f.v = cast_rational_to_float (x);
        f.f = real_op (f.f);
        ret->value = (void *)((imm_int_t)f.f);
        ret->attr.type = imm_int;
        break;
      }
    case imm_int:
      {
        *ret = *x;
        break;
      }
    default:
      PANIC ("Type not match, type is %d\n", x->attr.type);
    }

  return ret;
}

object_t _floor (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // For integers, floor is the number itself
  if (x->attr.type == imm_int) {
    *ret = *x;
    return ret;
  }

  // For real numbers, we need to implement floor manually
  if (x->attr.type == real) {
    real_t f;
    f.v = (uintptr_t)x->value;

    // Extract sign, exponent, and mantissa
    int sign = f.negative;
    int exponent = f.exponent - 127; // Bias for single precision
    uint32_t mantissa = f.mantissa | (1 << 23); // Add implicit leading 1

    if (exponent >= 23) {
      // Number is an integer or larger than fractional precision
      *ret = *x;
      return ret;
    } else if (exponent < 0) {
      // Number is between -1 and 1
      if (sign) {
        // Negative number: floor is -1 for numbers between -1 and 0
        ret->value = (void *)(intptr_t)-1;
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
      } else {
        // Positive number: floor is 0 for numbers between 0 and 1
        ret->value = (void *)(intptr_t)0;
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
      }
      return ret;
    } else {
      // Number has fractional part
      // Shift to get integer part
      uint32_t shift = 23 - exponent;
      uint32_t integer_part = mantissa >> shift;

      if (sign) {
        // For negative numbers, floor is integer_part + 1 if there's a fractional part
        // Check if there's a fractional part
        uint32_t fractional_mask = (1 << shift) - 1;
        if ((mantissa & fractional_mask) != 0) {
          integer_part += 1;
        }
        ret->value = (void *)(intptr_t)-(int32_t)integer_part;
      } else {
        ret->value = (void *)(intptr_t)integer_part;
      }
      ret->attr.type = imm_int;
      ret->attr.gc = FREE_OBJ;
      return ret;
    }
  }

  // For other types, use op_dispatch if available
  // But on bare metal, we may not have floorf
  PANIC("floor not fully implemented for this type\n");
  return NULL;
}

object_t _floor_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  // For now, implement for integers only
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in floor/\n");
    }
    // Floor division for integers is regular division when both are integers
    imm_int_t result = a / b;
    // Adjust for negative numbers to ensure floor behavior
    if (a % b != 0 && ((a < 0) ^ (b < 0))) {
      result--;
    }
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  // For other types, we need to convert to float and use floorf
  // This is a simplified implementation
  real_t fx, fy;
  if (x->attr.type == imm_int) {
    fx.f = (float)(imm_int_t)x->value;
  } else if (x->attr.type == real) {
    fx.v = (uintptr_t)x->value;
  } else {
    PANIC("floor/ not implemented for this type\n");
    return NULL;
  }

  if (y->attr.type == imm_int) {
    fy.f = (float)(imm_int_t)y->value;
  } else if (y->attr.type == real) {
    fy.v = (uintptr_t)y->value;
  } else {
    PANIC("floor/ not implemented for this type\n");
    return NULL;
  }

  // Implement floor division without floorf
  // For integers, we already handled
  // For real numbers, we can use integer operations on the IEEE 754 representation
  // This is complex, so for now, we'll use a simplified approach
  // Convert to integer if possible
  int exponent_x = fx.exponent - 127;
  int exponent_y = fy.exponent - 127;
  if (exponent_x >= 23 && exponent_y >= 23) {
    // Both numbers are integers
    imm_int_t a = (imm_int_t)fx.f;
    imm_int_t b = (imm_int_t)fy.f;
    if (b == 0) {
      PANIC("Division by zero in floor/\n");
    }
    imm_int_t result = a / b;
    // Adjust for negative numbers
    if (a % b != 0 && ((a < 0) ^ (b < 0))) {
      result--;
    }
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  } else {
    // For non-integers, we need a more complex implementation
    // For now, panic
    PANIC("floor/ for non-integer real numbers not implemented\n");
    return NULL;
  }
}

object_t _ceiling (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // For integers, ceiling is the number itself
  if (x->attr.type == imm_int) {
    *ret = *x;
    return ret;
  }

  // For real numbers, ceiling is -floor(-x)
  if (x->attr.type == real) {
    // Create a copy of x with negated sign
    real_t f;
    f.v = (uintptr_t)x->value;
    f.negative = !f.negative;

    Object neg_x;
    neg_x.attr.type = real;
    neg_x.attr.gc = FREE_OBJ;
    neg_x.value = (void *)(uintptr_t)f.v;

    // Compute floor of -x
    Object floor_neg;
    _floor(vm, &floor_neg, &neg_x);

    // ceiling(x) = -floor(-x)
    if (floor_neg.attr.type == imm_int) {
      imm_int_t val = (imm_int_t)floor_neg.value;
      ret->value = (void *)(intptr_t)-val;
      ret->attr.type = imm_int;
      ret->attr.gc = FREE_OBJ;
    } else {
      // Should not happen if floor returns integer
      PANIC("ceiling: floor did not return integer\n");
    }
    return ret;
  }

  PANIC("ceiling not fully implemented for this type\n");
  return NULL;
}

object_t _truncate (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  if (_int_gt (x, &zero)) // x > 0
    {
      return _floor (vm, ret, x);
    }
  else // x <= 0
    {
      return _ceiling (vm, ret, x);
    }
}

object_t _round (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // For integers, round is the number itself
  if (x->attr.type == imm_int) {
    *ret = *x;
    return ret;
  }

  // For real numbers, round to nearest integer
  if (x->attr.type == real) {
    real_t f;
    f.v = (uintptr_t)x->value;

    // Get floor and ceiling
    Object floor_val, ceil_val;
    _floor(vm, &floor_val, x);
    _ceiling(vm, &ceil_val, x);

    imm_int_t floor_int = (imm_int_t)floor_val.value;
    imm_int_t ceil_int = (imm_int_t)ceil_val.value;

    // Compute distance to floor and ceiling
    // Since we can't subtract floats easily, we'll use the original value
    // This is a simplified approach
    // For proper rounding, we need to look at the fractional part
    // For now, always round towards positive infinity for tie-breaking
    // This is not correct, but better than nothing

    // Check if the number is exactly halfway between two integers
    // This is complex without floating point operations
    // For now, use floor for positive numbers, ceiling for negative numbers
    if (f.negative) {
      ret->value = (void *)(intptr_t)ceil_int;
    } else {
      ret->value = (void *)(intptr_t)floor_int;
    }
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("round not fully implemented for this type\n");
  return NULL;
}

object_t _rationalize (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  // For integers, they're already rational
  if (x->attr.type == imm_int) {
    *ret = *x;
    return ret;
  }

  // For real numbers, we need to convert to a rational approximation
  // This is a simplified implementation
  if (x->attr.type == real) {
    real_t f;
    f.v = (uintptr_t)x->value;
    // For now, just return the number as is
    // In a real implementation, we'd find the best rational approximation
    *ret = *x;
    return ret;
  }

  PANIC("rationalize not fully implemented for this type\n");
  return NULL;
}

object_t _floor_quotient (vm_t vm, object_t ret, immu_object_t x,
                          immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers, floor quotient is the same as floor division
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in floor-quotient\n");
    }
    imm_int_t result = a / b;
    // Adjust for negative numbers
    if (a % b != 0 && ((a < 0) ^ (b < 0))) {
      result--;
    }
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("floor-quotient not fully implemented\n");
  return NULL;
}

object_t _floor_remainder (vm_t vm, object_t ret, immu_object_t x,
                           immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers, floor remainder is a - b * floor_quotient(a, b)
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in floor-remainder\n");
    }
    imm_int_t quotient = a / b;
    if (a % b != 0 && ((a < 0) ^ (b < 0))) {
      quotient--;
    }
    imm_int_t result = a - b * quotient;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("floor-remainder not fully implemented\n");
  return NULL;
}

object_t _truncate_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers, truncate division is regular division
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in truncate/\n");
    }
    imm_int_t result = a / b;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  // For real numbers, use truncf
  real_t fx, fy;
  if (x->attr.type == imm_int) {
    fx.f = (float)(imm_int_t)x->value;
  } else if (x->attr.type == real) {
    fx.v = (uintptr_t)x->value;
  } else {
    PANIC("truncate/ not implemented for this type\n");
    return NULL;
  }

  if (y->attr.type == imm_int) {
    fy.f = (float)(imm_int_t)y->value;
  } else if (y->attr.type == real) {
    fy.v = (uintptr_t)y->value;
  } else {
    PANIC("truncate/ not implemented for this type\n");
    return NULL;
  }

  // Implement truncate division without truncf
  // Similar to floor division, but truncate towards zero
  int exponent_x = fx.exponent - 127;
  int exponent_y = fy.exponent - 127;
  if (exponent_x >= 23 && exponent_y >= 23) {
    // Both numbers are integers
    imm_int_t a = (imm_int_t)fx.f;
    imm_int_t b = (imm_int_t)fy.f;
    if (b == 0) {
      PANIC("Division by zero in truncate/\n");
    }
    imm_int_t result = a / b;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  } else {
    PANIC("truncate/ for non-integer real numbers not implemented\n");
    return NULL;
  }
}

object_t _truncate_quotient (vm_t vm, object_t ret, immu_object_t x,
                             immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers, truncate quotient is the same as truncate division
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in truncate-quotient\n");
    }
    imm_int_t result = a / b;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("truncate-quotient not fully implemented\n");
  return NULL;
}

object_t _truncate_remainder (vm_t vm, object_t ret, immu_object_t x,
                              immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers, truncate remainder is a - b * truncate_quotient(a, b)
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t b = (imm_int_t)y->value;
    if (b == 0) {
      PANIC("Division by zero in truncate-remainder\n");
    }
    imm_int_t quotient = a / b;
    imm_int_t result = a - b * quotient;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("truncate-remainder not fully implemented\n");
  return NULL;
}

object_t _numerator (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
      // For integers, numerator is the number itself
      *ret = *x;
      break;
    case rational_pos:
    case rational_neg:
      // Extract numerator from rational
      // Assuming rational is stored in a certain format
      // This is a placeholder implementation
      PANIC("numerator for rational not implemented\n");
      break;
    case real:
      // For real numbers, numerator is not well-defined
      PANIC("numerator not defined for real numbers\n");
      break;
    default:
      PANIC("numerator not implemented for this type\n");
  }
  return ret;
}

object_t _denominator (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
      // For integers, denominator is 1
      ret->value = (void *)(intptr_t)1;
      ret->attr.type = imm_int;
      ret->attr.gc = FREE_OBJ;
      break;
    case rational_pos:
    case rational_neg:
      // Extract denominator from rational
      // This is a placeholder implementation
      PANIC("denominator for rational not implemented\n");
      break;
    case real:
      // For real numbers, denominator is not well-defined
      PANIC("denominator not defined for real numbers\n");
      break;
    default:
      PANIC("denominator not implemented for this type\n");
  }
  return ret;
}

object_t _is_exact_integer (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
      *ret = GLOBAL_REF(true_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;

        // Check if exponent >= 23 (no fractional part in single precision)
        int exponent = f.exponent - 127;
        if (exponent >= 23) {
          *ret = GLOBAL_REF(true_const);
        } else if (exponent < 0) {
          // Number is between -1 and 1
          if (f.mantissa == 0 && f.exponent == 0) {
            // Zero
            *ret = GLOBAL_REF(true_const);
          } else {
            *ret = GLOBAL_REF(false_const);
          }
        } else {
          // Check if fractional bits are all zero
          uint32_t shift = 23 - exponent;
          uint32_t fractional_mask = (1 << shift) - 1;
          if ((f.mantissa & fractional_mask) == 0) {
            *ret = GLOBAL_REF(true_const);
          } else {
            *ret = GLOBAL_REF(false_const);
          }
        }
        break;
      }
    case rational_pos:
    case rational_neg:
      // Check if denominator is 1
      // This is a placeholder
      PANIC("is-exact-integer for rational not implemented\n");
      break;
    default:
      *ret = GLOBAL_REF(false_const);
  }
  return ret;
}

object_t _is_finite (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF(true_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // Check if exponent is 255 (infinity or NaN)
        if (f.exponent == 255) {
          *ret = GLOBAL_REF(false_const);
        } else {
          *ret = GLOBAL_REF(true_const);
        }
        break;
      }
    default:
      *ret = GLOBAL_REF(false_const);
  }
  return ret;
}

object_t _is_infinite (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF(false_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // Check if exponent is 255 and mantissa is 0 (infinity)
        if (f.exponent == 255 && f.mantissa == 0) {
          *ret = GLOBAL_REF(true_const);
        } else {
          *ret = GLOBAL_REF(false_const);
        }
        break;
      }
    default:
      *ret = GLOBAL_REF(false_const);
  }
  return ret;
}

object_t _is_nan (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  switch (x->attr.type) {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF(false_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // Check if exponent is 255 and mantissa is non-zero (NaN)
        if (f.exponent == 255 && f.mantissa != 0) {
          *ret = GLOBAL_REF(true_const);
        } else {
          *ret = GLOBAL_REF(false_const);
        }
        break;
      }
    default:
      *ret = GLOBAL_REF(false_const);
  }
  return ret;
}

object_t _is_zero (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // xx is a number
  switch (x->attr.type)
    {
    case real:
    case imm_int:
      {
        *ret = x->value ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        imm_int_t check = (0xFFFF0000 & (imm_int_t)x->value);
        *ret = check ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);
        break;
      }
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    default:
      {
        PANIC ("Type not match, type is %d\n", x->attr.type);
      }
    }

  return ret;
}

object_t _is_positive (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  *ret
    = _int_gt (x, &zero) ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);

  return ret;
}

object_t _is_negative (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);
  Object zero = {.attr = {.type = imm_int, .gc = FREE_OBJ}, .value = 0};

  *ret
    = _int_gt (x, &zero) ? GLOBAL_REF (false_const) : GLOBAL_REF (true_const);

  return ret;
}

bool __is_odd (vm_t vm, immu_object_t x, char *op)
{
  bool ret = false;

  switch (x->attr.type)
    {
    case real:
      {
        Object tmp = {0};
        real_t a;
        a.v = (uintptr_t)x->value;

        if (255 == a.exponent)
          {
            if (0 == a.mantissa)
              PANIC ("%s: Wrong type argument - infinity!", op);
            else
              PANIC ("%s: Wrong type argument - Nan!", op);
          }
        else if (0 == a.exponent)
          {
            if (0 == a.mantissa) // exactly 0
              ret = false;
            else // a subnormal number
              PANIC ("%s: Wrong type argument - %f", op, a.f);
          }
        else 
          {
            // Check if it's an integer without using floorf
            // Check if exponent >= 23 (no fractional part)
            int exponent = a.exponent - 127;
            if (exponent >= 23) {
              // It's an integer
              // Check the least significant bit
              // For integers, the mantissa's LSB corresponds to the integer's LSB
              // Shift amount to get to the units place
              uint32_t shift = exponent - 23;
              if (shift >= 32) {
                // Number is too large to have fractional part, always even?
                // For now, assume even
                ret = false;
              } else {
                uint32_t lsb = (a.mantissa >> shift) & 1;
                ret = lsb;
              }
            } else {
              PANIC ("%s: Wrong type argument - not an integer!", op);
            }
          }

        break;
      }
    case imm_int:
      {
        ret = (uintptr_t)x->value & 1;
        break;
      }
    case complex_inexact:
    case complex_exact:
      {
        PANIC ("Complex not implemented yet\n");
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        PANIC ("Rational not implemented yet!\n");
        break;
      }
    default:
      {
        PANIC ("Type not match, type is %d\n", x->attr.type);
      }
    }

  return ret;
}

object_t _is_odd (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  *ret = __is_odd (vm, x, "odd?") ? GLOBAL_REF (true_const)
    : GLOBAL_REF (false_const);

  return ret;
}

object_t _is_even (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  *ret = __is_odd (vm, x, "even?") ? GLOBAL_REF (false_const)
                                   : GLOBAL_REF (true_const);

  return ret;
}

object_t _square (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  // Multiply x by itself
  // For integers
  if (x->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    imm_int_t result = a * a;
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  // For real numbers
  if (x->attr.type == real) {
    real_t f;
    f.v = (uintptr_t)x->value;
    float result = f.f * f.f;
    real_t res;
    res.f = result;
    ret->value = (void *)(uintptr_t)res.v;
    ret->attr.type = real;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("square not implemented for this type\n");
  return NULL;
}

object_t _sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  // For integers, use integer square root
  if (x->attr.type == imm_int) {
    imm_int_t a = (imm_int_t)x->value;
    if (a < 0) {
      PANIC("sqrt of negative integer\n");
    }

    // Integer square root using binary search
    imm_int_t low = 0, high = a;
    if (high > 46340) high = 46340; // sqrt(2^31-1) ~ 46340

    while (low <= high) {
      imm_int_t mid = (low + high) / 2;
      imm_int_t square = mid * mid;

      if (square == a) {
        ret->value = (void *)(intptr_t)mid;
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        return ret;
      } else if (square < a) {
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }

    // Not a perfect square, return real approximation
    // For now, return the floor of the square root as integer
    ret->value = (void *)(intptr_t)high;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  // For real numbers, we need to implement square root
  // This is complex, so for now, convert to integer if possible
  if (x->attr.type == real) {
    real_t f;
    f.v = (uintptr_t)x->value;
    if (f.negative) {
      PANIC("sqrt of negative number\n");
    }

    // Check if it's an integer
    Object is_int;
    _is_exact_integer(vm, &is_int, x);
    if (is_int.value == GLOBAL_REF(true_const).value) {
      // Convert to integer and use integer sqrt
      Object floor_val;
      _floor(vm, &floor_val, x);
      imm_int_t int_val = (imm_int_t)floor_val.value;
      return _sqrt(vm, ret, &floor_val);
    }

    PANIC("sqrt for non-integer real numbers not implemented\n");
  }

  PANIC("sqrt not implemented for this type\n");
  return NULL;
}

object_t _exact_integer_sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER(x);

  if (x->attr.type == imm_int) {
    imm_int_t n = (imm_int_t)x->value;
    if (n < 0) {
      PANIC("exact-integer-sqrt of negative integer\n");
    }

    // Find floor(sqrt(n)) using integer square root
    imm_int_t s = 0;
    if (n > 0) {
      imm_int_t low = 0, high = n;
      if (high > 46340) high = 46340;

      while (low <= high) {
        imm_int_t mid = (low + high) / 2;
        imm_int_t square = mid * mid;

        if (square == n) {
          s = mid;
          break;
        } else if (square < n) {
          s = mid; // Keep the largest mid where square < n
          low = mid + 1;
        } else {
          high = mid - 1;
        }
      }
    }

    imm_int_t r = n - s * s;

    // We need to return two values: s and r
    // For now, just return s
    // In Scheme, exact-integer-sqrt returns two values
    // This implementation is incomplete
    ret->value = (void *)(intptr_t)s;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  PANIC("exact-integer-sqrt only for exact integers\n");
  return NULL;
}

object_t _expt (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER(x);
  VALIDATE_NUMBER(y);

  // For integers
  if (x->attr.type == imm_int && y->attr.type == imm_int) {
    imm_int_t base = (imm_int_t)x->value;
    imm_int_t exp = (imm_int_t)y->value;
    if (exp < 0) {
      // For negative exponents, result is not an integer
      // Convert to float
      float result = powf((float)base, (float)exp);
      real_t res;
      res.f = result;
      ret->value = (void *)(uintptr_t)res.v;
      ret->attr.type = real;
      ret->attr.gc = FREE_OBJ;
      return ret;
    }
    imm_int_t result = 1;
    for (imm_int_t i = 0; i < exp; i++) {
      result *= base;
    }
    ret->value = (void *)(intptr_t)result;
    ret->attr.type = imm_int;
    ret->attr.gc = FREE_OBJ;
    return ret;
  }

  // For real numbers
  real_t fx, fy;
  if (x->attr.type == imm_int) {
    fx.f = (float)(imm_int_t)x->value;
  } else if (x->attr.type == real) {
    fx.v = (uintptr_t)x->value;
  } else {
    PANIC("expt not implemented for this type\n");
    return NULL;
  }

  if (y->attr.type == imm_int) {
    fy.f = (float)(imm_int_t)y->value;
  } else if (y->attr.type == real) {
    fy.v = (uintptr_t)y->value;
  } else {
    PANIC("expt not implemented for this type\n");
    return NULL;
  }

  // Implement pow without powf
  // For integer exponents, use repeated multiplication
  // Check if y is an integer
  int exponent_y = fy.exponent - 127;
  if (exponent_y >= 23) {
    // y is an integer
    imm_int_t exp_int = (imm_int_t)fy.f;
    if (exp_int >= 0) {
      // Positive integer exponent
      real_t result;
      result.f = 1.0f;
      for (imm_int_t i = 0; i < exp_int; i++) {
        result.f *= fx.f;
      }
      ret->value = (void *)(uintptr_t)result.v;
      ret->attr.type = real;
      ret->attr.gc = FREE_OBJ;
      return ret;
    } else {
      // Negative integer exponent
      imm_int_t abs_exp = -exp_int;
      real_t result;
      result.f = 1.0f;
      for (imm_int_t i = 0; i < abs_exp; i++) {
        result.f *= fx.f;
      }
      result.f = 1.0f / result.f;
      ret->value = (void *)(uintptr_t)result.v;
      ret->attr.type = real;
      ret->attr.gc = FREE_OBJ;
      return ret;
    }
  } else {
    // Non-integer exponent
    PANIC("expt for non-integer exponents not implemented\n");
    return NULL;
  }
}
