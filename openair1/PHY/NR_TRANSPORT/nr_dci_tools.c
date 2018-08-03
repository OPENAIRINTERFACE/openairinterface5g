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

/*! \file PHY/NR_TRANSPORT/nr_dci_tools.c
 * \brief
 * \author
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */

#include "nr_dci.h"

void nr_fill_dci_and_dlsch(PHY_VARS_gNB *gNB,
                           int frame,
                           int subframe,
                           gNB_rxtx_proc_t *proc,
                           NR_gNB_DCI_ALLOC_t *dci_alloc,
                           nfapi_nr_dl_config_request_pdu_t *pdu)
{
	NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
	uint32_t *dci_pdu = dci_alloc->dci_pdu;
	nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &pdu->dci_dl_pdu.dci_dl_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &pdu->dci_dl_pdu.pdcch_params_rel15;
	nfapi_nr_config_request_t *cfg = &gNB->gNB_config;

  uint16_t N_RB = fp->initial_bwp_dl.N_RB;
  uint8_t fsize = 0;

  /// Payload generation
  switch(params_rel15->dci_format) {

    case NFAPI_NR_DL_DCI_FORMAT_1_0:
      switch(params_rel15->rnti_type) {
        case NFAPI_NR_RNTI_RA:
          // Freq domain assignment
          fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i))&1)<<i;
          // Time domain assignment
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(4-i))&1)<<i;
          //MCS
          for (int i=0; i<5; i++)
            *dci_pdu |= ((pdu_rel15->mcs>>(5-i))&1)<<i;
          // TB scaling
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->tb_scaling>>(2-i))&1)<<i;
          
          break;

      }
      break;

    case NFAPI_NR_UL_DCI_FORMAT_0_0:
      break;
  }

  /// rest of DCI alloc
  memcpy((void*)&dci_alloc->pdcch_params, (void*)params_rel15, sizeof(nfapi_nr_dl_config_pdcch_parameters_rel15_t));
  dci_alloc->size = nr_get_dci_size(NFAPI_NR_DL_DCI_FORMAT_1_0,
                        NFAPI_NR_RNTI_RA,
                        &fp->initial_bwp_dl,
                        cfg);
  
  

/*	if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_1_0) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_1_0;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
		if (rel15->rnti_type == NFAPI_NR_RNTI_C
		 || rel15->rnti_type == NFAPI_NR_RNTI_CS
		 || rel15->rnti_type == NFAPI_NR_RNTI_new) {

		} else if (rel15->rnti_type == NFAPI_NR_RNTI_P) {

		} else if (rel15->rnti_type == NFAPI_NR_RNTI_SI) {

		} else if (rel15->rnti_type == NFAPI_NR_RNTI_RA) {

		} else if (rel15->rnti_type == NFAPI_NR_RNTI_TC) {

		} else {
			AssertFatal(1==0, "[nr_fill_dci_and_dlsch] Incorrect DCI Format(%d) and RNTI Type(%d) combination",rel15->dci_format, rel15->rnti_type);
		}
	} else if (rel15->dci_format == NFAPI_NR_UL_DCI_FORMAT_0_0) {
		dci_alloc->format = NFAPI_NR_UL_DCI_FORMAT_0_0;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_ul ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_1_1;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_UL_DCI_FORMAT_0_1) {
		dci_alloc->format = NFAPI_NR_UL_DCI_FORMAT_0_1;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_ul ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_2_0) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_2_0;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_2_1) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_2_1;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_2_2) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_2_2;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
	} else if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_2_3) {
		dci_alloc->format = NFAPI_NR_DL_DCI_FORMAT_2_3;
		dci_alloc->size = nr_get_dci_size(rel15->dci_format, rel15->rnti_type, &fp->initial_bwp_params_dl ,cfg);
	} else {
		AssertFatal(1==0, "[nr_fill_dci_and_dlsch] Incorrect DCI Format(%d)",rel15->dci_format);
	}*/

	return;
}
