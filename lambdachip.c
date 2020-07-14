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

#include "lambdachip.h"

vm_t lambdachip_init(void)
{
  vm_t vm = (vm_t)os_malloc(sizeof(struct LambdaVM));

#ifndef LAMBDACHIP_LINUX
  os_flash_init();
#endif

  init_ram_heap();
  primitives_init();
  // NOTE: The allocated vm object will never be freed.
  vm_init(vm);
  stdio_init();
  return vm;
}

void lambdachip_start(void)
{
  vm_t vm = lambdachip_init();

  // TODO: Print lambdachip version information
  os_printk("Welcome to Lambdachip! %s\n", get_platform_info());
  os_printk("Author: Mu Lei known as Nala Ginrut <mulei@gnu.org>\n");
  os_printk("Type `help' to get help\n");

  run_shell(vm);
}
