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


void bit8_rxdemux(int length,int offset)
{

  int i;
  short *rx1_ptr =  (short *)&PHY_vars->rx_vars[1].RX_DMA_BUFFER[offset];
  short *rx0_ptr =  (short *)&PHY_vars->rx_vars[0].RX_DMA_BUFFER[offset];
  char  *rx0_ptr2 = (char *)(&PHY_vars->rx_vars[0].RX_DMA_BUFFER[offset]);
  //  short tmp;
  short r0,i0,r1,i1;

  //  printf("demuxing: %d,%d\n",length,offset);

  //  printf("%x %x\n",PHY_vars->chsch_data[0].CHSCH_f_sync[0],    PHY_vars->chsch_data[0].CHSCH_f_sync[1]);

  for (i=0; i<length; i++) {




    r0= (short)(rx0_ptr2[i<<2]);        // Re 0

    i0= (short)(rx0_ptr2[(i<<2)+1]);  // Im 0

    r1= (short)(rx0_ptr2[(i<<2)+2]);    // Re 1

    i1= (short)(rx0_ptr2[(i<<2)+3]);  // Im 1

    rx0_ptr[(i<<1)] = r0;
    rx0_ptr[(i<<1)+1] =i0;
    rx1_ptr[i<<1] =r1;
    rx1_ptr[(i<<1)+1] =i1;
  }

  //  printf("%x %x\n",PHY_vars->chsch_data[0].CHSCH_f_sync[0],    PHY_vars->chsch_data[0].CHSCH_f_sync[1]);

}
