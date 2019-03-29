/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
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
     AssertFatal(1==0,"init_ue_paging_info: Ns is %u\n",Ns);

   return(0);
}
