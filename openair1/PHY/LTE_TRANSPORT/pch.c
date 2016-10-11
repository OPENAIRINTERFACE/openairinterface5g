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
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "assertions.h"

const unsigned int Ttab[4] = {32,64,128,256};

// This function implements the initialization of paging parameters for UE (See Section 7, 36.304)
// It must be called after setting IMSImod1024 during UE startup and after receiving SIB2
int init_ue_paging_info(PHY_VARS_UE *ue, long defaultPagingCycle, long nB) {

   LTE_DL_FRAME_PARMS *fp = &ue->frame_parms;

   unsigned int T         = Ttab[defaultPagingCycle];
   unsigned int N         = (nB<=2) ? T : (T>>(nB-2));
   unsigned int Ns        = (nB<2)  ? (1<<(2-nB)) : 1;
   unsigned int UE_ID     = ue->IMSImod1024;
   unsigned int i_s       = (UE_ID/N)%Ns;

   
   ue->PF = (T/N) * (UE_ID % N);

   // This implements Section 7.2 from 36.304
   if (Ns==1)
     ue->PO = (fp->frame_type==FDD) ? 9 : 0; 
   else if (Ns==2)
     ue->PO = (fp->frame_type==FDD) ? (4+(5*i_s)) : (5*i_s); 
   else if (Ns==4)
     ue->PO = (fp->frame_type==FDD) ? (4*(i_s&1)+(5*(i_s>>1))) : ((i_s&1)+(5*(i_s>>1))); 
   else
     AssertFatal(1==0,"init_ue_paging_info: Ns is %d\n",Ns);

   return(0);
}
