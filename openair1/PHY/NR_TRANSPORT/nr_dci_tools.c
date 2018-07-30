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
	uint32_t *dci_pdu = &dci_alloc->dci_pdu[0];
	nfapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15 = &pdu->dci_dl_pdu.dci_dl_pdu_rel15;
	nfapi_nr_config_request_t *cfg = &gNB->gNB_config;

	dci_alloc->L        = rel15->aggregation_level;

	if (rel15->dci_format == NFAPI_NR_DL_DCI_FORMAT_1_0) {
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
	}

	return;
}
