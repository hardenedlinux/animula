#ifndef __LAMBDACHIP_BYTECODE_H__
#define __LAMBDACHIP_BYTECODE_H__
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

#include "types.h"

/*
 * The design of this VM ISA is inspired by:
 * <<PICOBIT: A Compact Scheme System for Microcontrollers>>
 * Authors: Vincent St-Amour and Marc Feeley

 * I've redesigned it for better extensibility. The object encoding was redesigned
 * as well, and it's expected NOT ONLY SUPPORT Scheme. -- by NalaGinrut

 * Terms:
 * ---------------------------------------------------------------
 TOS: top of stack
 HSA: Higher Segment Address == pc + ss16[x] + 128
 * ---------------------------------------------------------------

 * Limits:
 * 1. The max size of stack frame is 256

 -> single encode
 0000xxxx                      Push constant x
 0001xxxx                      Push element from ss[x]
 0010xxxx                      Push next bytecode to global offset xxxx
 0011xxxx                      Set TOS to global
 0100xxxx                      Call closure at TOS with x arguments
 0101xxxx                      Jump to closure at TOS with x arguments
 0110xxxx                      Jump to entry point at address pc + x
 0111xxxx                      Go to address pc + x if TOS is false

 -> double encoding (start from 1010)
 1010 0000 xxxxxxxx             Push constant u8 x
 1010 0001 xxxxxxxx             Long jump to HSA
 1010 0010 xxxxxxxx             Go to HSA if TOS is false, ss32[x] is the offset
 1010 0011 xxxxxxxx             Build a closure with entry point ss[x] to TOS
 1010 0100 xxxxxxxx             Pop constant from ss[x] to TOS
 1010 0101 xxxxxxxx             Push constant s8 x
 1010 0110 xxxxxxxx             Reserved
 1010 0111 xxxxxxxx             Reserved
 1010 1000 xxxxxxxx             Reserved
 1010 1001 xxxxxxxx             Reserved
 1010 1110 xxxxxxxx             Reserved
 1010 1111 xxxxxxxx             Reserved

 -> triple encoding (start from 1011)
 1011 0000 nnnnnnnn xxxxxxxx    Call procedure at address ss[x] with n args
 1011 0001 xxxxxxxx xxxxxxxx    Push s16 constant x
 1011 0010 xxxxxxxx iiiiiiii    Vector ss[x] ref i
 1011 0011 xxxxxxxx xxxxxxxx    Push u16 constant x
 1011 0100 xxxxxxxx xxxxxxxx    Reserved
 1011 0101 xxxxxxxx xxxxxxxx    Reserved
 1011 0110 xxxxxxxx xxxxxxxx    Reserved
 1011 0111 xxxxxxxx xxxxxxxx    Reserved
 1011 1000 xxxxxxxx xxxxxxxx    Reserved
 1011 1001 xxxxxxxx xxxxxxxx    Reserved
 1011 1110 xxxxxxxx xxxxxxxx    Reserved
 1011 1111 xxxxxxxx xxxxxxxx    Reserved

 -> quadruple
 1000 0000 xxxxxxxx iiiiiiii vvvvvvvv   Vector ss[x] set i with v

 -> Reserved
 1001

 -> Speical encoding
 1100xxxx                       Basic primitives (+, return, get-cont, ...)
 1101xxxx xxxxxxxx              Extended primitives

 1110xxxx [xxxxxxxx]            Object encoding
 11100000                       Boolean false
 11100001                       Boolean true
 11100010 tttttttt              General object: t is the type, see object.h
 11100011 cccccccc              Char object: c: 0~256 (no UTF-8)

 1111xxxx xxxxxxxx              Reserved
 11111111                       Halt
*/

#define SINGLE_ENCODE(bc) (((bc).type >= 0) && ((bc).type <= 0b0111))
#define DOUBLE_ENCODE(bc) (0b1010 == (bc).type)
#define TRIPLE_ENCODE(bc) (0b1011 == (bc).type)
#define QUADRUPLE_ENCODE(bc) (0b0010 == (bc).type)
#define IS_SPECIAL(bc) (0b1100 & (bc).type)

// small encode
#define PUSH_SMALL_CONST 0
#define LOAD_SS_SMALL 1

// single encode
#define PUSH_GLOBAL     0b0010
#define SET_GLOBAL      0b0011
#define CALL_CLOSURE    0b0100
#define JUMP_CLOSURE    0b0101
#define JUMP            0b0110
#define JUMP_FALSE      0b0111

// double encode
#define PUSH_8BIT_CONST 0b0000
#define LONG_JUMP       0b0001
#define LONG_JUMP_TOS   0b0010
#define MAKE_CLOSURE    0b0011

// triple encode
#define CALL_PROC        0b0000
#define PUSH_16BIT_CONST 0b0001
#define VEC_REF          0b0010

// quadruple encoding
#define VEC_SET   0b0000

// special encoding
#define PRIMITIVE       0b1100
#define OBJECT          0b1110
#define HALT            0xff

typedef enum encode_type
  {
   SMALL, SINGLE, DOUBLE, TRIPLE, QUADRUPLE, SPECIAL
  } encode_t;

// FIXME: tweak bit-fields order by bits endian

typedef union ByteCode8
{
  struct
  {
#if defined LAMBDACHIP_BITS_LITTLE
    unsigned type: 4;
    unsigned data: 4;
#elif defined LAMBDACHIP_BITS_BIG
    unsigned data: 4;
    unsigned type: 4;
#endif
  };
  u8_t all;
} __packed bytecode8_t;

typedef union ByteCode16
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1: 8;
    unsigned bc2: 8;
  };
  struct
  {
    unsigned _: 4;
    unsigned type: 4;
    unsigned data: 8;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc2: 8;
    unsigned bc1: 8;
  };
  struct
  {
    unsigned data: 8;
    unsigned type: 4;
    unsigned _: 4;
  };
#endif
  u16_t all;
} __packed bytecode16_t;

typedef union ByteCode24
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1: 8;
    unsigned bc2: 8;
    unsigned bc3: 8;
  };
  struct
  {
    unsigned _: 4;
    unsigned type: 4;
    unsigned data: 16;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc3: 8;
    unsigned bc2: 8;
    unsigned bc1: 8;
  };
  struct
  {
    unsigned data: 16;
    unsigned type: 4;
    unsigned _: 4;
  };
#endif
} __packed bytecode24_t;

typedef union ByteCode32
{
#if defined LAMBDACHIP_BITS_LITTLE
  struct
  {
    unsigned bc1: 8;
    unsigned bc2: 8;
    unsigned bc3: 8;
    unsigned bc4: 8;
  };
  struct
  {
    unsigned _: 4;
    unsigned type: 4;
    unsigned data: 24;
  };
#elif defined LAMBDACHIP_BITS_BIG
  struct
  {
    unsigned bc4: 8;
    unsigned bc3: 8;
    unsigned bc2: 8;
    unsigned bc1: 8;
  };
  struct
  {
    unsigned data: 24;
    unsigned type: 4;
    unsigned _: 4;
  };
#endif
  u32_t all;
} __packed bytecode32_t;

#endif // End of __LAMBDACHIP_BYTECODE_H__
