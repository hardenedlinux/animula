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

#include "lambdachip.h"
#include "vos/drivers/gpio.h"
#ifdef LAMBDACHIP_ZEPHYR
// #include "file_operation.h"
#endif /* LAMBDACHIP_ZEPHYR */

vm_t lambdachip_init (void)
{
  vm_t vm = (vm_t)os_malloc (sizeof (struct LambdaVM));

#ifndef LAMBDACHIP_LINUX
  os_flash_init ();
#endif

  init_ram_heap ();
  primitives_init ();
  // NOTE: The allocated vm object will never be freed.
  vm_init (vm);
  stdio_init ();
  gc_init ();
  return vm;
}

void lambdachip_clean (vm_t vm)
{
  printf ("clean 1\n");
  vm_clean (vm);
  printf ("clean 2\n");
  primitives_clean ();
  printf ("clean 3\n");
  stdio_clean ();
  printf ("clean 4\n");
  gc_clean ();
  printf ("clean 5\n");
}

vm_t lambdachip_start (lef_loader_t lef_loader)
{
  vm_t vm = lambdachip_init ();

  // TODO: Print lambdachip version information
  VM_DEBUG ("Welcome to Lambdachip! %s\n", get_platform_info ());
  VM_DEBUG ("Author: Mu Lei known as Nala Ginrut <mulei@gnu.org>\n");
  VM_DEBUG ("Type `help' to get help\n");

  lef_t lef = LEF_LOAD (lef_loader);

  if (!strncmp (lef->sig, "LEF", 3))
    {
      printf ("load lef\n");
      vm_load_lef (vm, lef);
      printf ("run vm\n");
      vm_run (vm);
      printf ("clean lef\n");
      free_lef (lef);
      printf ("clean lef done\n");
    }
  else
    {
      os_printk ("No LEF loaded, run kernel shell!\n");
      run_shell (vm);
    }

  return vm;
}
