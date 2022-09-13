
/*  Copyright (C) 2020-2021
 *         Rafael Lee <rafaellee.img@gmail.com>
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
#include "bytevector.h"
#include "debug.h" // PANIC
#include "gc.h"
#include "object.h"

object_t _make_bytevector (vm_t vm, object_t ret, object_t count,
                           object_t datum)
{
  VALIDATE (count, imm_int);
  VALIDATE (datum, imm_int);

  mut_bytevector_t bv = NEW_INNER_OBJ (mut_bytevector);
  bv->attr.gc = GEN_1_OBJ;
  imm_int_t cnt = (imm_int_t) (count->value);
  bv->size = (imm_int_t) (count->value);
  u8_t *buf = (u8_t *)GC_MALLOC (cnt);
  bv->vec = buf;
  imm_int_t v = (imm_int_t) (datum->value);
  // FIXME: use memset instead
  for (int i = 0; i < cnt; i++)
    {
      *(buf + i) = v;
    }
  ret->attr.type = mut_bytevector;
  ret->attr.gc = GEN_1_OBJ;
  ret->value = (void *)bv;
  return ret;
}

object_t _bytevector_length (vm_t vm, object_t ret, object_t bv)
{
  VALIDATE_BYTEVECTOR (bv); // mutable and immutable bytevector
  ret->attr.type = imm_int;
  ret->attr.gc = GEN_1_OBJ;
  ret->value = (void *)(((bytevector_t) (bv->value))->size);
  return ret;
}

object_t _bytevector_u8_ref (vm_t vm, object_t ret, object_t bv, object_t index)
{
  VALIDATE_BYTEVECTOR (bv); // mutable and immutable bytevector
  VALIDATE (index, imm_int);
  ret->attr.type = imm_int;
  ret->attr.gc = GEN_1_OBJ;
  imm_int_t idx = (imm_int_t) (index->value);
  bytevector_t p = (bytevector_t) (bv->value);
  if (idx < MIN_UINT16 || idx > p->size - 1)
    {
      PANIC ("Bytevector index %d is out of range [%d, %d]\n", idx, MIN_UINT16,
             p->size - 1);
    }
  // imm_int_t v = (idx + (u8_t *)(((bytevector_t*) (bv->value))->vec));
  // imm_int_t v = 0;
  imm_int_t v = (p->vec)[idx]; // u8_t is in the range of imm_int_t(s32)
  ret->value = (void *)v;
  return ret;
}

object_t _bytevector_u8_set (vm_t vm, object_t ret, object_t bv, object_t index,
                             object_t datum)
{
  VALIDATE (bv, mut_bytevector);
  VALIDATE (index, imm_int);
  VALIDATE (datum, imm_int);
  ret->attr.type = imm_int;
  ret->attr.gc = GEN_1_OBJ;
  imm_int_t idx = (imm_int_t) (index->value);
  bytevector_t p = (bytevector_t) (bv->value);

  if (idx < MIN_UINT16 || idx > p->size - 1)
    {
      PANIC ("Bytevector index %d is out of range [0, %d]\n", idx, p->size - 1);
    }
  imm_int_t v = (imm_int_t) (datum->value);
  if (v < MIN_UINT8 || v > MAX_UINT8)
    {
      PANIC ("Bytevector value %d should be in range [0, %d]", v, MAX_UINT8);
    }
  (p->vec)[idx] = v;
  ret->value = (void *)v;
  return ret;
}

// (define a #u8(1 2 3 4 5))
// (bytevector-copy a 2 4))
// ==> #u8(3 4)
object_t _bytevector_copy (vm_t vm, object_t ret, object_t bv, object_t start,
                           object_t end)
{
  VALIDATE_BYTEVECTOR (bv); // mutable and immutable bytevector
  VALIDATE (start, imm_int);
  VALIDATE (end, imm_int);
  bytevector_t p = (bytevector_t) (bv->value);
  imm_int_t initial = (imm_int_t) (start->value);
  imm_int_t final = (imm_int_t) (end->value);

  if (final < MIN_UINT16 || final > p->size || initial > final || initial < 0)
    {
      PANIC ("Bytevector range [%d, %d] is out of range [0, %d]\n", initial,
             final, p->size);
    }

  Object len
    = {.attr = {.gc = 0, .type = imm_int}, .value = (void *)(final - initial)};
  Object datum = {.attr = {.gc = 0, .type = imm_int}, .value = (void *)0};

  ret = _make_bytevector (vm, ret, &len, &datum);

  u8_t *buf = ((bytevector_t) (ret->value))->vec;
  os_memcpy (buf, (p->vec) + initial, final - initial);
  return ret;
}

// (define a (bytevector 1 2 3 4 5))
// (define b (bytevector 10 20 30 40 50))
// (bytevector-copy! b 1 a 0 2)
// b ==> #u8(10 1 2 40 50)
object_t _bytevector_copy_overwrite (vm_t vm, object_t ret, object_t dst,
                                     object_t index, object_t src,
                                     object_t src_start, object_t src_end)
{
  VALIDATE (dst, mut_bytevector);
  VALIDATE (index, imm_int);
  VALIDATE_BYTEVECTOR (src); // mutable and immutable bytevector
  VALIDATE (src_start, imm_int);
  VALIDATE (src_end, imm_int);

  bytevector_t bv_dst = (bytevector_t) (dst->value);
  bytevector_t bv_src = (bytevector_t) (src->value);
  imm_int_t dst_idx = (imm_int_t) (index->value);
  imm_int_t src_initial = (imm_int_t) (src_start->value);
  imm_int_t src_final = (imm_int_t) (src_end->value);

  imm_int_t dst_last_idx = dst_idx - src_initial + src_final;

  if ((src_initial > src_final) || (src_initial < MIN_UINT16)
      || (src_final > bv_src->size) || (dst_idx < MIN_UINT16)
      || (dst_last_idx > bv_dst->size))
    {
      PANIC ("Range error: range src[%d], dst[%d], cannot copy src[%d, %d] to "
             "dst[%d, %d]\n",
             bv_src->size, bv_dst->size, src_initial, src_final, dst_idx,
             dst_last_idx);
    }

  os_memcpy (bv_dst->vec + dst_idx, bv_src->vec + src_initial,
             src_final - src_initial);
  Object temp_obj = {.attr = {.gc = 0, .type = none}, .value = (void *)0};
  *ret = temp_obj;
  ret->attr.type = none; // return <unspecified>

  return ret;
}

object_t _bytevector_append (vm_t vm, object_t ret, object_t bv0, object_t bv1)
{
  VALIDATE_BYTEVECTOR (bv0); // mutable and immutable bytevector
  VALIDATE_BYTEVECTOR (bv1); // mutable and immutable bytevector
  bytevector_t vec0 = (bytevector_t) (bv0->value);
  bytevector_t vec1 = (bytevector_t) (bv1->value);

  Object tmp0 = {.attr = {.gc = 0, .type = imm_int}, .value = (void *)0};
  Object size0
    = {.attr = {.gc = 0, .type = imm_int}, .value = (void *)vec0->size};
  Object size1
    = {.attr = {.gc = 0, .type = imm_int}, .value = (void *)vec1->size};
  Object size_all = {.attr = {.gc = 0, .type = imm_int},
                     .value = (void *)vec0->size + vec1->size};

  Object new_bv = {0};
  _make_bytevector (vm, &new_bv, &size_all, &tmp0);
  _bytevector_copy_overwrite (vm, ret, &new_bv, &tmp0, bv0, &tmp0, &size0);
  _bytevector_copy_overwrite (vm, ret, &new_bv, &size0, bv1, &tmp0, &size1);

  *ret = new_bv;
  return ret;
}