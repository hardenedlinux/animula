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

/* ---------------------------------------------------------------------
 * Rational helpers
 *
 * Per types.h, a rational is packed into a single 32bit immediate:
 *   sign:1 | numerator:15 | denominator:16   (little-endian bitfield order)
 * The sign is ALSO carried redundantly by the object type tag
 * (rational_pos == positive, rational_neg == negative); we always trust
 * the type tag as canonical, since that's what the rest of the runtime
 * (encoding table in object.h) treats as the source of truth.
 * ------------------------------------------------------------------- */

static inline rational_t rat_decode (immu_object_t x)
{
  rational_t r;
  r.value = (u32_t)(uintptr_t)x->value;
  return r;
}

static inline bool rat_is_negative (immu_object_t x)
{
  return x->attr.type == rational_neg;
}

static inline numerator_t rat_num (immu_object_t x)
{
  return (numerator_t)rat_decode (x).numerator;
}

static inline denominator_t rat_denom (immu_object_t x)
{
  return (denominator_t)rat_decode (x).denominator;
}

/* Build a normalized rational object into ret. num/denom are unsigned
 * magnitudes; is_neg gives the sign. denom == 0 is a caller error. denom
 * == 1 degenerates to an exact integer, which we return as imm_int since
 * that's the canonical exact-integer representation in this encoding. */
static object_t rat_make (object_t ret, int32_t num, uint32_t denom,
                           bool is_neg)
{
  if (denom == 0)
    PANIC ("Rational: zero denominator\n");

  if (num == 0)
    {
      ret->value = (void *)(intptr_t)0;
      ret->attr.type = imm_int;
      ret->attr.gc = FREE_OBJ;
      return ret;
    }

  /* reduce by gcd so the packed 15/16 bit fields don't overflow
   * needlessly and the value stays canonical */
  uint32_t a = (uint32_t)num, b = denom, t;
  while (b != 0)
    {
      t = a % b;
      a = b;
      b = t;
    }
  uint32_t g = a ? a : 1;
  uint32_t rn = (uint32_t)num / g;
  uint32_t rd = denom / g;

  if (rd == 1)
    {
      ret->value = (void *)(intptr_t)(is_neg ? -(imm_int_t)rn
                                              : (imm_int_t)rn);
      ret->attr.type = imm_int;
      ret->attr.gc = FREE_OBJ;
      return ret;
    }

  if (rn > 0x7FFF || rd > 0xFFFF)
    PANIC ("Rational: numerator/denominator overflow after reduction "
           "(%u/%u) -- needs promotion to bignum rational, not "
           "supported yet\n",
           rn, rd);

  rational_t r;
  r.sign = is_neg ? 1 : 0;
  r.numerator = rn;
  r.denominator = rd;
  ret->value = (void *)(uintptr_t)r.value;
  ret->attr.type = is_neg ? rational_neg : rational_pos;
  ret->attr.gc = FREE_OBJ;
  return ret;
}

static inline float rat_to_float (immu_object_t x)
{
  rational_t r = rat_decode (x);
  float v = (float)r.numerator / (float)r.denominator;
  return rat_is_negative (x) ? -v : v;
}

static inline bool num_is_negative (immu_object_t x)
{
  switch (x->attr.type)
    {
    case imm_int:
      return ((imm_int_t)x->value) < 0;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // -0.0 has the sign bit set but is not "negative" as a number
        bool is_zero_val = (f.exponent == 0 && f.mantissa == 0);
        return f.negative && !is_zero_val;
      }
    case rational_neg:
      return true;
    case rational_pos:
      return false;
    default:
      PANIC ("sign not defined for type %d\n", x->attr.type);
      return false;
    }
}

static inline bool num_is_positive (immu_object_t x)
{
  switch (x->attr.type)
    {
    case imm_int:
      return ((imm_int_t)x->value) > 0;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        bool is_zero_val = (f.exponent == 0 && f.mantissa == 0);
        return (!f.negative) && !is_zero_val;
      }
    case rational_pos:
      // rat_make() collapses a zero numerator to imm_int 0, so any
      // surviving rational_pos object is strictly > 0.
      return true;
    case rational_neg:
      return false;
    default:
      PANIC ("sign not defined for type %d\n", x->attr.type);
      return false;
    }
}

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
        /* R7RS: applying a transcendental like sin/cos/sqrt to an exact
         * rational produces an INEXACT result -- it must stay `real',
         * never get truncated down to an integer. */
        real_t f;
        f.f = rat_to_float (x);
        f.f = real_op (f.f);
        ret->value = (void *)(uintptr_t)f.v;
        ret->attr.type = real;
        ret->attr.gc = FREE_OBJ;
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

/* floor/ceiling of a plain C float without libm's floorf/ceilf (bare-metal
 * friendly: only relies on ordinary float<->int conversion + comparison,
 * both plain hardware/soft-float ops, not library calls). */
static inline float float_floor (float v)
{
  imm_int_t t = (imm_int_t)v; // truncates toward zero
  float tf = (float)t;
  return (tf > v) ? (tf - 1.0f) : tf;
}

static inline float float_ceiling (float v)
{
  imm_int_t t = (imm_int_t)v;
  float tf = (float)t;
  return (tf < v) ? (tf + 1.0f) : tf;
}

object_t _floor (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      // exact integer: floor is itself (stays exact)
      *ret = *x;
      return ret;

    case real:
      {
        // inexact argument -> inexact result (R7RS exactness contagion)
        real_t f;
        f.v = (uintptr_t)x->value;
        real_t out;
        out.f = float_floor (f.f);
        ret->value = (void *)(uintptr_t)out.v;
        ret->attr.type = real;
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    case rational_pos:
    case rational_neg:
      {
        // exact rational -> exact integer
        imm_int_t num = (imm_int_t)rat_num (x);
        imm_int_t denom = (imm_int_t)rat_denom (x);
        imm_int_t q = num / denom;
        if (rat_is_negative (x) && (num % denom != 0))
          q += 1; // truncated division rounds toward zero; adjust to -inf
        ret->value = (void *)(intptr_t)(rat_is_negative (x) ? -q : q);
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    default:
      PANIC ("floor not implemented for this type\n");
      return NULL;
    }
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

  switch (x->attr.type)
    {
    case imm_int:
      *ret = *x;
      return ret;

    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        real_t out;
        out.f = float_ceiling (f.f);
        ret->value = (void *)(uintptr_t)out.v;
        ret->attr.type = real;
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    case rational_pos:
    case rational_neg:
      {
        imm_int_t num = (imm_int_t)rat_num (x);
        imm_int_t denom = (imm_int_t)rat_denom (x);
        imm_int_t q = num / denom;
        if (!rat_is_negative (x) && (num % denom != 0))
          q += 1; // truncated division rounds toward zero; adjust to +inf
        ret->value = (void *)(intptr_t)(rat_is_negative (x) ? -q : q);
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    default:
      PANIC ("ceiling not implemented for this type\n");
      return NULL;
    }
}

object_t _truncate (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  // truncate rounds toward zero: floor for non-negatives, ceiling for
  // negatives.
  return num_is_negative (x) ? _ceiling (vm, ret, x) : _floor (vm, ret, x);
}

object_t _round (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      *ret = *x;
      return ret;

    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        float v = f.f;
        float fl = float_floor (v);
        float frac = v - fl; // in [0, 1)

        float rounded;
        if (frac < 0.5f)
          rounded = fl;
        else if (frac > 0.5f)
          rounded = fl + 1.0f;
        else
          {
            // exactly halfway: round to even
            imm_int_t fi = (imm_int_t)fl;
            rounded = ((fi & 1) == 0) ? fl : fl + 1.0f;
          }

        real_t out;
        out.f = rounded;
        ret->value = (void *)(uintptr_t)out.v;
        ret->attr.type = real; // inexact in, inexact out
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    case rational_pos:
    case rational_neg:
      {
        // Exact rational -> exact integer, round-half-to-even, done with
        // pure integer arithmetic so exactness is never compromised.
        imm_int_t num = (imm_int_t)rat_num (x);
        imm_int_t denom = (imm_int_t)rat_denom (x);
        imm_int_t q = num / denom;
        imm_int_t r = num % denom;
        imm_int_t two_r = 2 * r;

        imm_int_t mag;
        if (two_r < denom)
          mag = q;
        else if (two_r > denom)
          mag = q + 1;
        else
          mag = ((q & 1) == 0) ? q : q + 1; // tie: round to even

        ret->value = (void *)(intptr_t)(rat_is_negative (x) ? -mag : mag);
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        return ret;
      }

    default:
      PANIC ("round not implemented for this type\n");
      return NULL;
    }
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
      {
        imm_int_t n = (imm_int_t)rat_num (x);
        ret->value = (void *)(intptr_t)(rat_is_negative (x) ? -n : n);
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        break;
      }
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
      {
        /* denominator is always positive by construction */
        ret->value = (void *)(intptr_t)(imm_int_t)rat_denom (x);
        ret->attr.type = imm_int;
        ret->attr.gc = FREE_OBJ;
        break;
      }
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
      /* Well-formed rationals are kept in lowest terms by rat_make(),
       * so denominator == 1 never survives as a rational_pos/neg object
       * (it collapses to imm_int) -- but check anyway defensively. */
      *ret = (rat_denom (x) == 1) ? GLOBAL_REF (true_const)
                                  : GLOBAL_REF (false_const);
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
      {
        /* check via decoded bits, not raw pointer-null: -0.0 has its
         * sign bit set (a non-null bit pattern) but is still zero. */
        real_t f;
        f.v = (uintptr_t)x->value;
        *ret = (f.exponent == 0 && f.mantissa == 0)
                 ? GLOBAL_REF (true_const)
                 : GLOBAL_REF (false_const);
        break;
      }
    case imm_int:
      {
        *ret = (x->value == NULL) ? GLOBAL_REF (true_const)
                                   : GLOBAL_REF (false_const);
        break;
      }
    case rational_pos:
    case rational_neg:
      {
        /* A well-formed (reduced) rational should never actually be
         * zero -- rat_make() collapses numerator==0 to an imm_int 0 --
         * but stay defensive against a malformed encoding. */
        *ret = (rat_num (x) == 0) ? GLOBAL_REF (true_const)
                                   : GLOBAL_REF (false_const);
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

  *ret = num_is_positive (x) ? GLOBAL_REF (true_const)
                              : GLOBAL_REF (false_const);

  return ret;
}

object_t _is_negative (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  *ret = num_is_negative (x) ? GLOBAL_REF (true_const)
                              : GLOBAL_REF (false_const);

  return ret;
}

bool __is_odd (vm_t vm, immu_object_t x, char *op)
{
  bool ret = false;

  switch (x->attr.type)
    {
    case real:
      {
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
            int exponent = a.exponent - 127;
            // 24-bit mantissa including the implicit leading 1 bit
            uint32_t mantissa = a.mantissa | (1u << 23);

            if (exponent >= 23)
              {
                // value = mantissa << (exponent - 23), an exact integer.
                // Any left-shift beyond 0 forces a trailing zero bit, so
                // the result is even unless exponent is exactly 23.
                if (exponent == 23)
                  ret = mantissa & 1;
                else
                  ret = false; // even
              }
            else
              {
                // There IS room for a fractional part at this magnitude
                // -- but that doesn't mean this particular value has
                // one. Check whether the low bits (below the integer
                // part) are all zero.
                uint32_t shift = 23 - exponent;
                uint32_t frac_mask = (1u << shift) - 1;
                if ((mantissa & frac_mask) != 0)
                  PANIC ("%s: Wrong type argument - not an integer!", op);
                uint32_t lsb = (mantissa >> shift) & 1;
                ret = lsb;
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
        /* odd?/even? require an exact integer; a non-degenerate
         * rational (denominator != 1) is not an integer at all. */
        if (rat_denom (x) != 1)
          PANIC ("%s: Wrong type argument - not an integer!", op);
        ret = rat_num (x) & 1;
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
      // R7RS: (expt <exact-int> <negative-exact-int>) is EXACT --
      // it must stay a rational (1 / base^|exp|), never fall to float.
      if (base == 0)
        PANIC ("expt: division by zero (0 raised to a negative power)\n");
      imm_int_t abs_exp = -exp;
      imm_int_t denom_mag = 1;
      bool overflowed = false;
      for (imm_int_t i = 0; i < abs_exp; i++) {
        /* TODO: on overflow this should promote to arbi_int (bignum);
         * the bignum path is not implemented yet in this file. */
        if (denom_mag > MAX_INT32 / (base < 0 ? -base : base)) {
          overflowed = true;
          break;
        }
        denom_mag *= (base < 0 ? -base : base);
      }
      if (overflowed)
        PANIC ("expt: result denominator overflow -- needs bignum "
               "rational support, not implemented yet\n");
      bool result_neg = (base < 0) && (abs_exp & 1);
      return rat_make (ret, 1, (uint32_t)denom_mag, result_neg);
    }
    imm_int_t result = 1;
    for (imm_int_t i = 0; i < exp; i++) {
      /* TODO: no overflow check -- should promote to arbi_int (bignum)
       * on overflow instead of silently wrapping. Not implemented yet. */
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
