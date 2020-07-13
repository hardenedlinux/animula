#ifndef __LAMBDACHIP_SHELL_H__
#define __LAMBDACHIP_SHELL_H__
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
#include "__stdio.h"
#include "lef.h"
#include "storage.h"
#include "vm.h"

#define KSC_DESC_LEN    32
#define KSC_NAME_LEN    8
#define KSC_MAXARGS     8
#define KSC_WHITESPACE  "\t\r\n "

typedef int (*ksc_run_t)(int, char**, vm_t vm);

#define KSC_END  { {}, {}, NULL }

typedef struct KernelShellCommand
{
  const char name[KSC_NAME_LEN];
  const char desc[KSC_DESC_LEN];

  // return -1 to force kshell to exit
  ksc_run_t run;
} __packed ksc_t;

#define _E(x)   (-(0xE000 + x))

typedef enum return_value
  {
   /* EINVSZ = _E(1), */
   /* ENOMEM = _E(2), */
   /* EINVAL = _E(2), */
   OK = 0
  } retval_t;

void run_shell(vm_t vm);
#endif // End of __LAMBDACHIP_SHELL_H__
