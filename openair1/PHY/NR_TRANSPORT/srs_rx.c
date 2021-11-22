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

/*! \file PHY/NR_TRANSPORT/srs_rx.c
 * \brief Top-level routines for getting the SRS physical channel
 * \date 2021
 * \version 1.0
 */

#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "PHY/impl_defs_nr.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_gNB.h"
#include <openair1/PHY/CODING/nrSmallBlock/nr_small_block_defs.h>
#include "common/utils/LOG/log.h"

#include "nfapi/oai_integration/vendor_ext.h"

#include "T.h"

NR_gNB_SRS_t *new_gNB_srs(void){
  NR_gNB_SRS_t *srs;
  srs = (NR_gNB_SRS_t *)malloc16(sizeof(NR_gNB_SRS_t));
  srs->active = 0;
  return (srs);
}

int nr_find_srs(uint16_t rnti,
                int frame,
                int slot,
                PHY_VARS_gNB *gNB) {

  AssertFatal(gNB!=NULL,"gNB is null\n");
  int index = -1;

  for (int i=0; i<NUMBER_OF_NR_SRS_MAX; i++) {
    AssertFatal(gNB->srs[i]!=NULL,"gNB->srs[%d] is null\n",i);
    if ((gNB->srs[i]->active >0) &&
        (gNB->srs[i]->srs_pdu.rnti==rnti) &&
        (gNB->srs[i]->frame==frame) &&
        (gNB->srs[i]->slot==slot)) return(i);
    else if ((gNB->srs[i]->active == 0) && (index==-1)) index=i;
  }

  if (index==-1)
    LOG_E(MAC,"SRS list is full\n");

  return(index);
}

void nr_fill_srs(PHY_VARS_gNB *gNB,
                 int frame,
                 int slot,
                 nfapi_nr_srs_pdu_t *srs_pdu) {

  int id = nr_find_srs(srs_pdu->rnti,frame,slot,gNB);
  AssertFatal( (id>=0) && (id<NUMBER_OF_NR_SRS_MAX),
               "invalid id found for srs !!! rnti %04x id %d\n",srs_pdu->rnti,id);

  NR_gNB_SRS_t  *srs = gNB->srs[id];
  srs->frame = frame;
  srs->slot = slot;
  srs->active = 1;
  memcpy((void*)&srs->srs_pdu, (void*)srs_pdu, sizeof(nfapi_nr_srs_pdu_t));
}

void nr_get_srs_signal(PHY_VARS_gNB *gNB, int frame, int slot, nfapi_nr_srs_pdu_t *srs_pdu) {
  LOG_W(NR_PHY, "(%d.%d) nr_get_srs_signal is not implemented yet!\n", frame,slot);
}