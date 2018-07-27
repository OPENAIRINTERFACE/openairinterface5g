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

#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"

uint8_t get_pmi(uint8_t N_RB_DL, MIMO_mode_t mode, uint32_t pmi_alloc,uint16_t rb)
{
  /*
  MIMO_mode_t mode   = dlsch_harq->mimo_mode;
  uint32_t pmi_alloc = dlsch_harq->pmi_alloc;
  */

  switch (N_RB_DL) {
    case 6:   // 1 PRB per subband
      if (mode <= PUSCH_PRECODING1)
        return((pmi_alloc>>(rb<<1))&3);
      else
        return((pmi_alloc>>rb)&1);

      break;

    default:
    case 25:  // 4 PRBs per subband
      if (mode <= PUSCH_PRECODING1)
        return((pmi_alloc>>((rb>>2)<<1))&3);
      else
        return((pmi_alloc>>(rb>>2))&1);

      break;

    case 50: // 6 PRBs per subband
      if (mode <= PUSCH_PRECODING1)
        return((pmi_alloc>>((rb/6)<<1))&3);
      else
        return((pmi_alloc>>(rb/6))&1);

      break;

    case 100: // 8 PRBs per subband
      if (mode <= PUSCH_PRECODING1)
        return((pmi_alloc>>((rb>>3)<<1))&3);
      else
        return((pmi_alloc>>(rb>>3))&1);

      break;
  }
}
