#ifndef __LAMBDACHIP_BYTECODE_H__
#define __LAMBDACHIP_BYTECODE_H__
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

#include "os.h"
#include "types.h"

/*
 * The design of this VM ISA is inspired by:
 * <<PICOBIT: A Compact Scheme System for Microcontrollers>>
 * Authors: Vincent St-Amour and Marc Feeley

 * I've redesigned it for better extensibility. The object encoding was
 * redesigned as well, and it's expected NOT ONLY SUPPORT Scheme.
 * -- by NalaGinrut

 * Terms:
 * ---------------------------------------------------------------
 TOS: top of stack

 * ---------------------------------------------------------------

 * Limits:
 * 1. The max offset of local is 32+128=160
 * 2. The offset of free is 256
 * 3. The up frame of free is 16
 * 4. The code size is no more than 2^16, we can extend to around 16MB
      or even 4GB, but it's not a good idea, since we have to tweak
      the conditions and closures either. Do we really care about big RAMs
      in embedded world?
 * 5. Closure arity is no more than 64
 * 6. Closure frame-size is no more than 64
 * 7. Procedure entry is no more than 65KB, which means the LEF size is no more
      than 65KB.
 * 8. Globals are limited to the first 256 + 65536 bytes RAMs,
      8208 objects in total

 * New idea:
   * Combine all local/free to just one instruction, 0101mmmm, m for mode, make
     sure offset and up-frame is 256.


 -> single encode
 0000xxxx                       Ref local [x]
 0001xxxx                       Ref local [x + 16]
 0100xxxx                       Call local [x]
 0101xxxx                       Call local [x + 16]

 -> special double encoding
 0010 xxxx xxaaaaaa             Ref free up(fp)^a in offset x
 0011 xxxx xxaaaaaa             Call free up(fp)^a in offset x
 0110 xxxx xxaaaaaa             Assign TOS to up(fp)^a in offset x
 0111 xxxx xxxxxxxx             Assign TOS to local[x]

 -> double encoding (start from 1010)
 1010 0000 nnnnnnnn             Prelude with n args
 1010 0001 xxxxxxxx             Ref local[x + 32]
 1010 0010 xxxxxxxx             Call local[x + 32]
 1010 0011 xxxxxxxx             Assign TOS to global[x]
 1010 0100 xxxxxxxx             Ref global[x]
 1010 0101 xxxxxxxx             Call global[x]
 1010 0110 xxxxxxxx             Reserved
 1010 0111 xxxxxxxx             Reserved
 1010 1000 xxxxxxxx             Reserved
 1010 1001 xxxxxxxx             Reserved
 1010 1110 xxxxxxxx             Reserved
 1010 1111 xxxxxxxx             Reserved

 -> triple encoding (start from 1011)
 1011 0000 xxxxxxxx xxxxxxxx    Call proc at code[x]
 1011 0001 xxxxxxxx xxxxxxxx    Jump to code[x] when TOS is false
 1011 0010 xxxxxxxx xxxxxxxx    Jump to code[x] without condition
 1011 0011 xxxxxxxx iiiiiiii    Vector ss[x] ref i
 1011 0100 xxxxxxxx xxxxxxxx    Assign TOS to global[x + 128]
 1011 0101 xxxxxxxx xxxxxxxx    Ref global[x + 128]
 1011 0110 xxxxxxxx xxxxxxxx    Call global[x]
 1011 0111 xxxxxxxx xxxxxxxx    Reserved
 1011 1000 xxxxxxxx xxxxxxxx    Reserved
 1011 1001 xxxxxxxx xxxxxxxx    Reserved
 1011 1110 xxxxxxxx xxxxxxxx    Reserved
 1011 1111 xxxxxxxx xxxxxxxx    Reserved

 -> quadruple
 1000 0000 xxxxxxxx iiiiiiii vvvvvvvv   Vector ss[x] set i with v
 1000 0001 ffffffff aaaaaaaa aaaaaaaa   Closure on heap with frame, and the
                                        entry is code[a]
 1000 0010 ffffffff aaaaaaaa aaaaaaaa   Closure on stack

 -> Reserved
 1001

 -> Speical encoding
 1100xxxx                   Basic primitives
 1101xxxx xxxxxxxx          Extended primitives

 1110xxxx [xxxxxxxx]        Object encoding
 11100000                   Boolean false
 11100001                   Boolean true
 11100010 tttttttt          General object: t is the type, see object.h
 11100011 cccccccc          Char object: c: 0~255 (no UTF-8)
 11100100                   Empty list, '() in Scheme
 11100101                   None object, undefined in JS, unspecified in Scheme
 11100110 oooooooo oooooooo Symbol object, o is the offset in symbol table

 1111xxxx xxxxxxxx          Reserved
 11111111                   Halt
*/

#define SINGLE_ENCODE(bc)    (((bc).type >= 0) && ((bc).type <= 0b0111))
#define DOUBLE_ENCODE(bc)    (0b1010 == (bc).type)
#define TRIPLE_ENCODE(bc)    (0b1011 == (bc).type)
#define QUADRUPLE_ENCODE(bc) (0b1000 == (bc).type)
#define IS_SPECIAL(bc)       (0b1100 & (bc).type)

// single encode
#define LOCAL_REF         0b0000
#define LOCAL_REF_EXTEND  0b0001
#define FREE_REF          0b0010
#define CALL_LOCAL        0b0100
#define CALL_LOCAL_EXTEND 0b0101
#define CALL_FREE         0b0011
#define FREE_ASSIGN       0b0110
#define LOCAL_ASSIGN      0b0111

// double encode
#define PRELUDE           0b0000
#define LOCAL_REF_HIGH    0b0001
#define CALL_LOCAL_HIGH   0b0010
#define GLOBAL_VAR_ASSIGN 0b0011
#define GLOBAL_VAR_REF    0b0100
#define CALL_GLOBAL_VAR   0b0101

// triple encode
#define CALL_PROC                0b0000
#define F_JMP                    0b0001
#define JMP                      0b0010
#define VEC_REF                  0b0011
#define GLOBAL_VAR_ASSIGN_EXTEND 0b0100
#define GLOBAL_VAR_REF_EXTEND    0b0101
#define CALL_GLOBAL_VAR_EXTEND   0b0110

// quadruple encoding
#define VEC_SET          0b0000
#define CLOSURE_ON_HEAP  0b0001
#define CLOSURE_ON_STACK 0b0010

// special encoding
#define PRIMITIVE     0b1100
#define PRIMITIVE_EXT 0b1101
#define OBJECT        0b1110
#define CONTROL       0b1111
#define HALT          0b1111

typedef enum encode_type
{
  SMALL,
  SINGLE,
  DOUBLE,
  TRIPLE,
  QUADRUPLE,
  SPECIAL
} encode_t;

// FIXME: tweak bit-fields order by bits endian

typedef union ByteCode8
{
  struct
  {
#if defined LAMBDACHIP_BITS_LITTLE
    unsigned type : 4;
    unsigned data : 4;
#elif defined LAMBDACHIP_BITS_BIG
    unsigned data : 4;
    unsigned type : 4;
#else
#  error "Please define LAMBDACHIP_BITS_BIG or LAMBDACHIP_BITS_LITTLE"
#endif
  };
  u8_t all;
} __packed bytecode8_t;

typedef union ByteCode16
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1 : 8;
    unsigned bc2 : 8;
  };
  struct
  {
    unsigned _ : 4;
    unsigned type : 4;
    unsigned data : 8;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc2 : 8;
    unsigned bc1 : 8;
  };
  struct
  {
    unsigned data : 8;
    unsigned type : 4;
    unsigned _ : 4;
  };
#endif
  u16_t all;
} __packed bytecode16_t;

typedef union ByteCode24
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1 : 8;
    unsigned bc2 : 8;
    unsigned bc3 : 8;
  };
  struct
  {
    unsigned _ : 4;
    unsigned type : 4;
    unsigned data : 16;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc3 : 8;
    unsigned bc2 : 8;
    unsigned bc1 : 8;
  };
  struct
  {
    unsigned data : 16;
    unsigned type : 4;
    unsigned _ : 4;
  };
#endif
} __packed bytecode24_t;

typedef union ByteCode32
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1 : 8;
    unsigned bc2 : 8;
    unsigned bc3 : 8;
    unsigned bc4 : 8;
  };
  struct
  {
    unsigned _ : 4;
    unsigned type : 4;
    unsigned data : 24;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc4 : 8;
    unsigned bc3 : 8;
    unsigned bc2 : 8;
    unsigned bc1 : 8;
  };
  struct
  {
    unsigned data : 24;
    unsigned type : 4;
    unsigned _ : 4;
  };
#endif
  u32_t all;
} __packed bytecode32_t;

#endif // End of __LAMBDACHIP_BYTECODE_H__
