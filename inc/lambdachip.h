#ifndef __LAMBDACHIP_H__
#define __LAMBDACHIP_H__
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

#include "debug.h"
#include "gc.h"
#include "memory.h"
#include "os.h"
#include "primitives.h"
#include "shell.h"
#include "stdio.h"
#include "storage.h"
#include "types.h"
#include "vm.h"

typedef lef_t (*lef_loader_func_t) (const char *);

typedef struct LEF_Loader
{
  const char *filename;
  lef_loader_func_t loader;
} * lef_loader_t;

#define LEF_LOAD(ll) ((ll)->loader ((ll)->filename))

vm_t lambdachip_init (void);
void lambdachip_clean (vm_t vm);
vm_t lambdachip_start (lef_loader_t lef_loader);
#endif // End of __LAMBDACHIP_H__
