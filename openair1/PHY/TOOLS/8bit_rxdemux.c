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
