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

#include "stdio.h"

static char line_buf[LINE_BUF_SIZE] = {0};

char* read_line(const char* prompt)
{
  os_printk(prompt);
  for(int i = 0; i < LINE_BUF_SIZE; i++)
    {
      line_buf[i] = getchar();
      if ('\n' == line_buf[i])
        {
          line_buf[i] = '\0';
          break;
        }
    }

  return line_buf;
}

void stdio_init(void)
{
#if defined LAMBDACHIP_ZEPHYR
  console_init();
#endif
}
