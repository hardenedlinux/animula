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

static inline bool num_is_zero (immu_object_t x)
{
  switch (x->attr.type)
    {
    case imm_int:
      return x->value == NULL;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // both +0.0 and -0.0 have exponent == mantissa == 0
        return f.exponent == 0 && f.mantissa == 0;
      }
    case rational_pos:
    case rational_neg:
      /* A well-formed (reduced) rational should never actually be zero
       * -- rat_make() collapses numerator==0 to an imm_int 0 -- but
       * stay defensive against a malformed encoding. */
      return rat_num (x) == 0;
    default:
      PANIC ("zero? not defined for type %d\n", x->attr.type);
      return false;
    }
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

/* ---------------------------------------------------------------------
 * Small constructors / decoders
 *
 * Every numeric primitive in this file ends by stuffing a value into
 * `ret' along with its type tag and gc marker. Centralizing that here
 * removes dozens of copies of the same four lines and makes the exact
 * vs. inexact distinction explicit at every call site.
 * ------------------------------------------------------------------- */

static inline object_t mk_int (object_t ret, imm_int_t v)
{
  ret->value = (void *)(intptr_t)v;
  ret->attr.type = imm_int;
  ret->attr.gc = FREE_OBJ;
  return ret;
}

static inline object_t mk_real (object_t ret, float v)
{
  real_t r;
  r.f = v;
  ret->value = (void *)(uintptr_t)r.v;
  ret->attr.type = real;
  ret->attr.gc = FREE_OBJ;
  return ret;
}

static inline float to_float (immu_object_t x)
{
  real_t r;
  r.v = (uintptr_t)x->value;
  return r.f;
}

static inline bool float_is_nan_or_inf (immu_object_t x)
{
  real_t r;
  r.v = (uintptr_t)x->value;
  return r.exponent == 255;
}

/* NOTE: currently unused within this file -- kept for whichever
 * transcendental-function primitives (sin/cos/sqrt/...) end up calling
 * into it. Wire it up or drop it; a static function nothing calls is
 * dead weight. */
static object_t op_dispatch (vm_t vm, object_t ret, immu_object_t x,
                             real_op_t real_op)
{
  switch (x->attr.type)
    {
    case imm_int:
      // R7RS: a transcendental applied to an exact number is inexact.
      return mk_real (ret, real_op ((float)(imm_int_t)x->value));

    case real:
      return mk_real (ret, real_op (to_float (x)));

    case rational_pos:
    case rational_neg:
      return mk_real (ret, real_op (rat_to_float (x)));

    case complex_inexact:
    case complex_exact:
      PANIC ("Complex not implemented yet\n");
      return NULL;

    default:
      PANIC ("Type not match, type is %d\n", x->attr.type);
      return NULL;
    }
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

static inline bool float_is_integer (float v)
{
  return v == float_floor (v);
}

object_t _floor (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      *ret = *x; // exact integer: floor is itself
      return ret;

    case real:
      // inexact argument -> inexact result (R7RS exactness contagion)
      return mk_real (ret, float_floor (to_float (x)));

    case rational_pos:
    case rational_neg:
      {
        // exact rational -> exact integer
        imm_int_t num = (imm_int_t)rat_num (x);
        imm_int_t denom = (imm_int_t)rat_denom (x);
        imm_int_t q = num / denom;
        if (rat_is_negative (x) && (num % denom != 0))
          q += 1; // truncated division rounds toward zero; adjust to -inf
        return mk_int (ret, rat_is_negative (x) ? -q : q);
      }

    default:
      PANIC ("floor not implemented for this type\n");
      return NULL;
    }
}

/* ---------------------------------------------------------------------
 * floor/ and truncate/ family
 *
 * All six of floor-quotient / floor-remainder / floor/ / truncate-quotient
 * / truncate-remainder / truncate/ boil down to the same two integer
 * divisions (round-toward-negative-infinity and round-toward-zero); the
 * only thing that changes is which half of the result (quotient vs
 * remainder) gets returned.
 *
 * NOTE: R7RS defines floor/ and truncate/ as returning TWO values
 * (quotient and remainder). This VM's primitive calling convention here
 * only has room for a single `ret' object, so -- matching the previous
 * behavior of this file -- floor/ and truncate/ currently just return
 * the quotient, same as floor-quotient/truncate-quotient. If/when the
 * VM gains multiple-return-value support for primitives, these two
 * should be revisited to actually return both values.
 * ------------------------------------------------------------------- */

static void euclid_floor_divmod (const char *op, imm_int_t a, imm_int_t b,
                                  imm_int_t *q, imm_int_t *r)
{
  if (b == 0)
    PANIC ("Division by zero in %s\n", op);

  imm_int_t quot = a / b;
  imm_int_t rem = a % b;
  if (rem != 0 && ((a < 0) ^ (b < 0)))
    {
      quot -= 1;
      rem += b;
    }
  *q = quot;
  *r = rem;
}

static void euclid_truncate_divmod (const char *op, imm_int_t a, imm_int_t b,
                                     imm_int_t *q, imm_int_t *r)
{
  if (b == 0)
    PANIC ("Division by zero in %s\n", op);

  *q = a / b; // C's / and % already truncate toward zero
  *r = a % b;
}

static inline bool both_imm_int (immu_object_t x, immu_object_t y)
{
  return x->attr.type == imm_int && y->attr.type == imm_int;
}

object_t _floor_quotient (vm_t vm, object_t ret, immu_object_t x,
                          immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  if (!both_imm_int (x, y))
    PANIC ("floor-quotient not implemented for this type\n");

  imm_int_t q, r;
  euclid_floor_divmod ("floor-quotient", (imm_int_t)x->value,
                       (imm_int_t)y->value, &q, &r);
  return mk_int (ret, q);
}

object_t _floor_remainder (vm_t vm, object_t ret, immu_object_t x,
                           immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  if (!both_imm_int (x, y))
    PANIC ("floor-remainder not implemented for this type\n");

  imm_int_t q, r;
  euclid_floor_divmod ("floor-remainder", (imm_int_t)x->value,
                       (imm_int_t)y->value, &q, &r);
  return mk_int (ret, r);
}

object_t _floor_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  // see NOTE above: should return (quotient . remainder), currently
  // only returns the quotient, matching prior behavior.
  return _floor_quotient (vm, ret, x, y);
}

object_t _truncate_quotient (vm_t vm, object_t ret, immu_object_t x,
                             immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  if (!both_imm_int (x, y))
    PANIC ("truncate-quotient not implemented for this type\n");

  imm_int_t q, r;
  euclid_truncate_divmod ("truncate-quotient", (imm_int_t)x->value,
                          (imm_int_t)y->value, &q, &r);
  return mk_int (ret, q);
}

object_t _truncate_remainder (vm_t vm, object_t ret, immu_object_t x,
                              immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  if (!both_imm_int (x, y))
    PANIC ("truncate-remainder not implemented for this type\n");

  imm_int_t q, r;
  euclid_truncate_divmod ("truncate-remainder", (imm_int_t)x->value,
                          (imm_int_t)y->value, &q, &r);
  return mk_int (ret, r);
}

object_t _truncate_div (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  // see NOTE above: should return (quotient . remainder), currently
  // only returns the quotient, matching prior behavior.
  return _truncate_quotient (vm, ret, x, y);
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
      return mk_real (ret, float_ceiling (to_float (x)));

    case rational_pos:
    case rational_neg:
      {
        imm_int_t num = (imm_int_t)rat_num (x);
        imm_int_t denom = (imm_int_t)rat_denom (x);
        imm_int_t q = num / denom;
        if (!rat_is_negative (x) && (num % denom != 0))
          q += 1; // truncated division rounds toward zero; adjust to +inf
        return mk_int (ret, rat_is_negative (x) ? -q : q);
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
        float v = to_float (x);
        float fl = float_floor (v);
        float frac = v - fl; // in [0, 1)

        if (frac < 0.5f)
          return mk_real (ret, fl);
        if (frac > 0.5f)
          return mk_real (ret, fl + 1.0f);

        // exactly halfway: round to even
        imm_int_t fi = (imm_int_t)fl;
        return mk_real (ret, ((fi & 1) == 0) ? fl : fl + 1.0f);
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

        return mk_int (ret, rat_is_negative (x) ? -mag : mag);
      }

    default:
      PANIC ("round not implemented for this type\n");
      return NULL;
    }
}

object_t _rationalize (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = *x; // already exact/rational
      return ret;

    case real:
      /* TODO: not a real implementation -- R7RS rationalize should
       * return the simplest rational within the given tolerance
       * (typically via a Stern-Brocot / continued-fraction search).
       * This just returns the input unchanged. */
      *ret = *x;
      return ret;

    default:
      PANIC ("rationalize not implemented for this type\n");
      return NULL;
    }
}

object_t _numerator (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      *ret = *x; // numerator of an integer is itself
      return ret;
    case rational_pos:
    case rational_neg:
      {
        imm_int_t n = (imm_int_t)rat_num (x);
        return mk_int (ret, rat_is_negative (x) ? -n : n);
      }
    case real:
      PANIC ("numerator not defined for real numbers\n");
      return NULL;
    default:
      PANIC ("numerator not implemented for this type\n");
      return NULL;
    }
}

object_t _denominator (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      return mk_int (ret, 1); // denominator of an integer is 1
    case rational_pos:
    case rational_neg:
      return mk_int (ret, (imm_int_t)rat_denom (x)); // always positive
    case real:
      PANIC ("denominator not defined for real numbers\n");
      return NULL;
    default:
      PANIC ("denominator not implemented for this type\n");
      return NULL;
    }
}

object_t _is_exact_integer (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      *ret = GLOBAL_REF (true_const);
      break;
    case real:
      *ret = (!float_is_nan_or_inf (x) && float_is_integer (to_float (x)))
               ? GLOBAL_REF (true_const)
               : GLOBAL_REF (false_const);
      break;
    case rational_pos:
    case rational_neg:
      /* Well-formed rationals are kept in lowest terms by rat_make(),
       * so denominator == 1 never survives as a rational_pos/neg object
       * (it collapses to imm_int) -- but check anyway defensively. */
      *ret = (rat_denom (x) == 1) ? GLOBAL_REF (true_const)
                                  : GLOBAL_REF (false_const);
      break;
    default:
      *ret = GLOBAL_REF (false_const);
    }
  return ret;
}

object_t _is_finite (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF (true_const);
      break;
    case real:
      *ret = float_is_nan_or_inf (x) ? GLOBAL_REF (false_const)
                                     : GLOBAL_REF (true_const);
      break;
    default:
      *ret = GLOBAL_REF (false_const);
    }
  return ret;
}

object_t _is_infinite (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF (false_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // infinity: exponent field maxed out (255) and mantissa zero
        *ret = (f.exponent == 255 && f.mantissa == 0)
                 ? GLOBAL_REF (true_const)
                 : GLOBAL_REF (false_const);
        break;
      }
    default:
      *ret = GLOBAL_REF (false_const);
    }
  return ret;
}

object_t _is_nan (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
    case rational_pos:
    case rational_neg:
      *ret = GLOBAL_REF (false_const);
      break;
    case real:
      {
        real_t f;
        f.v = (uintptr_t)x->value;
        // NaN: exponent field maxed out (255) and mantissa non-zero
        *ret = (f.exponent == 255 && f.mantissa != 0)
                 ? GLOBAL_REF (true_const)
                 : GLOBAL_REF (false_const);
        break;
      }
    default:
      *ret = GLOBAL_REF (false_const);
    }
  return ret;
}

object_t _is_zero (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  if (x->attr.type == complex_inexact || x->attr.type == complex_exact)
    PANIC ("Complex not implemented yet\n");

  *ret = num_is_zero (x) ? GLOBAL_REF (true_const) : GLOBAL_REF (false_const);
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

        if (a.exponent == 255)
          PANIC ("%s: Wrong type argument - %s!", op,
                 a.mantissa == 0 ? "infinity" : "Nan");

        float v = a.f;
        if (!float_is_integer (v))
          PANIC ("%s: Wrong type argument - not an integer!", op);

        // A whole-number float at this magnitude can only be even --
        // float has 24 bits of precision, so beyond +-2^24 every
        // representable value is already a multiple of 2.
        ret = (v >= 16777216.0f || v <= -16777216.0f) ? false
                                                       : ((imm_int_t)v) & 1;
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

/* floor(sqrt(n)) for n >= 0, via binary search (bare-metal friendly: no
 * sqrtf needed). Capped at 46340 since 46341^2 already overflows a
 * 32bit imm_int_t. */
static imm_int_t int_sqrt_floor (imm_int_t n)
{
  if (n <= 0)
    return 0;

  imm_int_t low = 0, high = (n < 46341) ? n : 46340;
  imm_int_t best = 0;
  while (low <= high)
    {
      imm_int_t mid = low + (high - low) / 2;
      imm_int_t sq = mid * mid;
      if (sq == n)
        return mid;
      if (sq < n)
        {
          best = mid;
          low = mid + 1;
        }
      else
        high = mid - 1;
    }
  return best;
}

/* Newton's method sqrt for a plain C float, no libm sqrtf needed
 * (same bare-metal rationale as float_floor/float_ceiling above). */
static float float_sqrt (float v)
{
  if (v <= 0.0f)
    return 0.0f;

  float guess = (v > 1.0f) ? v : 1.0f;
  for (int i = 0; i < 20; i++)
    guess = 0.5f * (guess + v / guess);
  return guess;
}

object_t _square (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      {
        imm_int_t a = (imm_int_t)x->value;
        /* TODO: no overflow check -- should promote to arbi_int (bignum)
         * on overflow instead of silently wrapping. Not implemented. */
        return mk_int (ret, a * a);
      }

    case real:
      {
        float f = to_float (x);
        return mk_real (ret, f * f);
      }

    case rational_pos:
    case rational_neg:
      {
        int32_t n = (int32_t)rat_num (x);
        uint32_t d = rat_denom (x);
        return rat_make (ret, n * n, d * d, false); // square is never negative
      }

    default:
      PANIC ("square not implemented for this type\n");
      return NULL;
    }
}

object_t _sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  switch (x->attr.type)
    {
    case imm_int:
      {
        imm_int_t a = (imm_int_t)x->value;
        if (a < 0)
          PANIC ("sqrt of negative integer\n");
        imm_int_t r = int_sqrt_floor (a);
        // perfect square -> exact result; otherwise inexact
        return (r * r == a) ? mk_int (ret, r)
                             : mk_real (ret, float_sqrt ((float)a));
      }

    case real:
      {
        float v = to_float (x);
        if (v < 0.0f)
          PANIC ("sqrt of negative number\n");
        return mk_real (ret, float_sqrt (v));
      }

    case rational_pos:
      {
        // exact iff numerator and denominator are both perfect squares
        imm_int_t n = (imm_int_t)rat_num (x);
        imm_int_t d = (imm_int_t)rat_denom (x);
        imm_int_t sn = int_sqrt_floor (n);
        imm_int_t sd = int_sqrt_floor (d);
        return (sn * sn == n && sd * sd == d)
                 ? rat_make (ret, sn, sd, false)
                 : mk_real (ret, float_sqrt (rat_to_float (x)));
      }

    case rational_neg:
      PANIC ("sqrt of negative number\n");
      return NULL;

    default:
      PANIC ("sqrt not implemented for this type\n");
      return NULL;
    }
}

object_t _exact_integer_sqrt (vm_t vm, object_t ret, immu_object_t x)
{
  VALIDATE_NUMBER (x);

  if (x->attr.type != imm_int)
    PANIC ("exact-integer-sqrt only for exact integers\n");

  imm_int_t n = (imm_int_t)x->value;
  if (n < 0)
    PANIC ("exact-integer-sqrt of negative integer\n");

  // NOTE: R7RS exact-integer-sqrt returns TWO values (s and the
  // remainder n - s*s). This primitive's single-`ret' calling
  // convention can only carry one, so -- matching prior behavior --
  // this only returns `s'. Revisit if/when the VM gains multi-value
  // primitive returns.
  return mk_int (ret, int_sqrt_floor (n));
}

/* base^exp for exp >= 0, plain repeated multiplication (no powf). */
static float float_ipow (float base, imm_int_t exp)
{
  float result = 1.0f;
  for (imm_int_t i = 0; i < exp; i++)
    result *= base;
  return result;
}

object_t _expt (vm_t vm, object_t ret, immu_object_t x, immu_object_t y)
{
  VALIDATE_NUMBER (x);
  VALIDATE_NUMBER (y);

  // exact base, exact integer exponent -> exact result
  if (x->attr.type == imm_int && y->attr.type == imm_int)
    {
      imm_int_t base = (imm_int_t)x->value;
      imm_int_t exp = (imm_int_t)y->value;

      if (exp < 0)
        {
          // R7RS: (expt <exact-int> <negative-exact-int>) is EXACT --
          // it must stay a rational (1 / base^|exp|), never fall to
          // float.
          if (base == 0)
            PANIC ("expt: division by zero (0 raised to a negative "
                   "power)\n");

          imm_int_t abs_exp = -exp;
          imm_int_t abs_base = (base < 0) ? -base : base;
          imm_int_t denom_mag = 1;
          for (imm_int_t i = 0; i < abs_exp; i++)
            {
              /* TODO: on overflow this should promote to arbi_int
               * (bignum); not implemented yet. */
              if (denom_mag > MAX_INT32 / abs_base)
                PANIC ("expt: result denominator overflow -- needs "
                       "bignum rational support, not implemented yet\n");
              denom_mag *= abs_base;
            }
          bool result_neg = (base < 0) && (abs_exp & 1);
          return rat_make (ret, 1, (uint32_t)denom_mag, result_neg);
        }

      imm_int_t result = 1;
      for (imm_int_t i = 0; i < exp; i++)
        /* TODO: no overflow check -- should promote to arbi_int
         * (bignum) instead of silently wrapping. Not implemented. */
        result *= base;
      return mk_int (ret, result);
    }

  // Any inexact operand, or a mix -> inexact result.
  float bx;
  switch (x->attr.type)
    {
    case imm_int:
      bx = (float)(imm_int_t)x->value;
      break;
    case real:
      bx = to_float (x);
      break;
    case rational_pos:
    case rational_neg:
      bx = rat_to_float (x);
      break;
    default:
      PANIC ("expt not implemented for this type\n");
      return NULL;
    }

  float by;
  switch (y->attr.type)
    {
    case imm_int:
      by = (float)(imm_int_t)y->value;
      break;
    case real:
      by = to_float (y);
      break;
    default:
      PANIC ("expt not implemented for this type\n");
      return NULL;
    }

  if (!float_is_integer (by))
    PANIC ("expt for non-integer exponents not implemented\n");

  imm_int_t exp_int = (imm_int_t)by;
  float p = float_ipow (bx, (exp_int < 0) ? -exp_int : exp_int);
  return mk_real (ret, (exp_int >= 0) ? p : 1.0f / p);
}
