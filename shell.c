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

#include "shell.h"

static int show_help (int argc, char **argv, vm_t vm);
static int serial_load (int argc, char **argv, vm_t vm);
static int flash_test (int argc, char **argv, vm_t vm);
static int etest (int argc, char **argv, vm_t vm);
static int run_program (int argc, char **argv, vm_t vm);

#define KSC_CNT 10
static const ksc_t kernel_shell_cmd[]
  = {{"help", "List all commands", show_help},
     {"sload", "Load LEF from serial port", serial_load},
     {"ftest", "Flash test", flash_test},
     {"etest", "Endian test", etest},
     {"run_prog", "Run stored program", run_program},
     KSC_END};

static int show_help (int argc, char **argv, vm_t vm)
{
  for (int i = 0; i < KSC_CNT; i++)
    {
      ksc_t ksc = kernel_shell_cmd[i];

      if (NULL == ksc.run)
        break;

      os_printk ("%s - %s\n", ksc.name, ksc.desc);
    }

  return 0;
}

static int etest (int argc, char **argv, vm_t vm)
{
  u32_t n = 0x01234567;
  u8_t *ptr = (u8_t *)&n;
  os_printk ("start\n");
  for (int i = 0; i < 4; i++)
    os_printk (" %2x", ptr[i]);
  os_printk ("\n");
  return 0;
}

static int serial_load (int argc, char **argv, vm_t vm)
{
  bool run = false;
  bool save = false;

#define SLOAD_HELP() os_printk ("Usage: sload [save | once | run]\n")

  if (argc < 2)
    {
      SLOAD_HELP ();
      return 0;
    }
  else if (!strncmp (argv[1], "save", 5))
    {
      save = true;
    }
  else if (!strncmp (argv[1], "once", 5))
    {
      save = false;
    }
  else if (!strncmp (argv[1], "run", 4))
    {
      // save = true;
      run = true;
    }
  else
    {
      SLOAD_HELP ();
      return 0;
    }

  // Load to RAM
  os_printk ("Ready for receiving LEF......\n");
  lef_t lef = load_lef_from_uart ();

  if (!lef)
    goto end;

  if (save)
    store_lef (lef, 0);

  if (run)
    {
      vm_load_lef (vm, lef);
      vm_run (vm);
    }

end:
  if (lef)
    free_lef (lef);
  os_printk ("Free LEF successfully!]\n");
  return 0;
}

static int flash_test (int argc, char **argv, vm_t vm)
{
  char buf[10] = {0};
  int offset = LAMBDACHIP_FLASH_AVAILABLE_OFFSET;
  os_printk ("offset: %d\n", offset);

  os_flash_write ("hello", offset, 6);
  os_flash_read (buf, offset, 6);
  os_printk ("flash: %s\n", buf);
  return 0;
}

static int run_program (int argc, char **argv, vm_t vm)
{
  os_printk ("Loading LEF from flash......\n");
  lef_t lef = load_lef_from_flash (0);

  vm_load_lef (vm, lef);
  vm_run (vm);

  if (lef)
    free_lef (lef);
  os_printk ("Free LEF successfully!]\n");

  return 0;
}

static int run_cmd (char *buf, vm_t vm)
{
  int argc = 0;
  char *argv[KSC_MAXARGS] = {0};
  int i = 0;

  // Parse the command buffer into whitespace-separated arguments
  argv[argc] = NULL;

  while (true)
    {
      // gobble whitespace
      while (*buf && strchr (KSC_WHITESPACE, *buf))
        *buf++ = 0;

      if (0 == *buf)
        break;

      // save and scan past next arg
      if (KSC_MAXARGS - 1 == argc)
        {
          os_printk ("Too many arguments (max %d)\n", KSC_MAXARGS);
          return 0;
        }

      argv[argc++] = buf;

      while (*buf && !strchr (KSC_WHITESPACE, *buf))
        buf++;
    }

  argv[argc] = 0;

  // Lookup and invoke the command
  if (0 == argc)
    return 0;

  for (i = 0; i < KSC_CNT; i++)
    {
      if (NULL == kernel_shell_cmd[i].run)
        break;
      else if (0 == strncmp (argv[0], kernel_shell_cmd[i].name, KSC_NAME_LEN))
        return kernel_shell_cmd[i].run (argc, argv, vm);
    }
  os_printk ("Unknown command '%s'\n", argv[0]);
  return 0;
}

static void ksc_error_handler (retval_t rv, const char *cmd)
{
  os_printk ("retval: %d\n,cmd: %s\n,len-cmd: %d\n", rv, cmd,
             (u8_t)os_strnlen (cmd, LINE_BUF_SIZE));
}

void run_shell (vm_t vm)
{
  char *cmd = NULL;
  retval_t rv = OK;

  while (true)
    {
      cmd = read_line ("^CHIP> ");

      if (NULL != cmd && (rv = run_cmd (cmd, vm)) < 0)
        {
          ksc_error_handler (rv, cmd);
          break;
        }
    }
}
