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
