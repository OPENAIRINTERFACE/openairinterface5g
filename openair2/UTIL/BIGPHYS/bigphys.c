/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

#include "defs.h"
#include "linux/module.h"

#ifdef BIGPHYSAREA
#ifdef ARCH_64
char *bigphys_ptr,*bigphys_current;
#else //ARCH_64
unsigned int bigphys_ptr,bigphys_current;
#endif //ARCH_64

// return pointer to memory in big physical area aligned to 16 bytes

void* bigphys_malloc(int n)
{



  int n2 = n + ((16-(n%16))%16);
#ifdef ARCH_64
  char *bigphys_old;
#else
  unsigned int bigphys_old;
#endif

  printk("[BIGPHYSAREA] Calling bigphys_malloc for %d (%d) bytes\n",n,n2);

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory @ %p\n",bigphys_current);
#endif
  bigphys_old = bigphys_current;
  bigphys_current += n2;

#ifdef ARCH_64
  printk("[BIGPHYSAREA] Allocated Memory top now @ %p\n",bigphys_current);
  return ((void *)(bigphys_old));
#else //ARCH_64
  //printk("[BIGPHYSAREA] Allocated Memory %d\n",bigphys_current-bigphys_ptr);
  return ((void *)(bigphys_old));
#endif //ARCH_64
}

EXPORT_SYMBOL(bigphys_malloc);
#endif



