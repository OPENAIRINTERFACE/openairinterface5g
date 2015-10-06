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
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"


void bit8_txmux(int length,int offset)
{

  int i;
  short  *dest,*dest2;




  for (i=0; i<length; i++) {

    dest = (short *)&PHY_vars->tx_vars[0].TX_DMA_BUFFER[i+offset];
    dest2 =   (short *)&PHY_vars->tx_vars[1].TX_DMA_BUFFER[i+offset];

    ((char *)dest)[0] = (char)(dest[0]>>BIT8_TX_SHIFT);
    ((char *)dest)[1] = (char)(dest[1]>>BIT8_TX_SHIFT);
    ((char *)dest)[2] = (char)(dest2[0]>>BIT8_TX_SHIFT);
    ((char *)dest)[3] = (char)(dest2[1]>>BIT8_TX_SHIFT);
  }


}
