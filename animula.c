/*  Copyright (C) 2020-2021
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

#include "animula.h"
#include "vos/drivers/gpio.h"
#ifdef ANIMULA_ZEPHYR
// #include "file_operation.h"
#endif /* ANIMULA_ZEPHYR */

vm_t animula_init (void)
{
  vm_t vm = (vm_t)os_malloc (sizeof (struct LambdaVM));

#ifndef ANIMULA_LINUX
  os_flash_init ();
#endif

  init_ram_heap ();
  primitives_init ();
  // NOTE: The allocated vm object will never be freed.
  vm_init (vm);
  stdio_init ();
  ANIMULA_GC_INIT ();
  return vm;
}

void animula_clean (vm_t vm)
{
  vm_clean (vm);
  primitives_clean ();
  stdio_clean ();
  GC_CLEAN ();
}

vm_t animula_start (lef_loader_t lef_loader)
{
  vm_t vm = animula_init ();

  // TODO: Print animula version information
  VM_DEBUG ("Welcome to Animula! %s\n", get_platform_info ());
  VM_DEBUG ("Author: Mu Lei known as Nala Ginrut <mulei@gnu.org>\n");
  VM_DEBUG ("Type `help' to get help\n");

  lef_t lef = LEF_LOAD (lef_loader);

  if (!strncmp (lef->sig, "LEF", 3))
    {
      vm_load_lef (vm, lef);
      vm_run (vm);
      free_lef (lef);
    }
  else
    {
      os_printk ("No LEF loaded, run kernel shell!\n");
      run_shell (vm);
    }

  return vm;
}
