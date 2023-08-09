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

/*********************************************************************
*
* FILENAME    :  dummy_functions.c
*
* MODULE      :  NR UE unit test
*
* DESCRIPTION :  it allows to define unused functions for unit tests.
*
************************************************************************/

#include <stddef.h>

#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/types.h"
#include "PHY/defs_gNB.h"
#include "../common/ran_context.h"
#include "../common/config/config_paramdesc.h"


/*****************variables****************************************/

RAN_CONTEXT_t RC;

uint32_t (*p_nr_ue_get_SR)(module_id_t module_idP,int CC_id,frame_t frameP,uint8_t eNB_id,uint16_t rnti, sub_frame_t subframe) = NULL;

/*****************functions****************************************/

lte_subframe_t subframe_select(LTE_DL_FRAME_PARMS *frame_parms,unsigned char subframe)
{
	return(0);
}

PRACH_RESOURCES_t *ue_get_rach(module_id_t module_idP, int CC_id,
			       frame_t frameP, uint8_t new_Msg3,
			       sub_frame_t subframe){ return(NULL);}

void ue_get_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
		sub_frame_t subframe, uint8_t eNB_index,
		uint8_t * ulsch_buffer, uint16_t buflen,
		uint8_t * access_mode){}

void Msg1_transmitted(module_id_t module_idP, uint8_t CC_id,
		      frame_t frameP, uint8_t eNB_id){}

void Msg3_transmitted(module_id_t module_idP, uint8_t CC_id,
		      frame_t frameP, uint8_t eNB_id){}

uint32_t nr_ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
		   uint8_t eNB_id, rnti_t rnti, sub_frame_t subframe){

	uint32_t value = 0;
	if (p_nr_ue_get_SR != NULL)
	{
	  value = (p_nr_ue_get_SR)(module_idP, CC_id, frameP, eNB_id, rnti, subframe);
	}
	return(value);}

void rrc_out_of_sync_ind(module_id_t Mod_idP, frame_t frameP, uint16_t eNB_index)
{}

UE_L2_STATE_t ue_scheduler(const module_id_t module_idP,
			   const frame_t rxFrameP,
			   const sub_frame_t rxSubframe,
			   const frame_t txFrameP,
			   const sub_frame_t txSubframe,
			   const lte_subframe_t direction,
			   const uint8_t eNB_index, const int CC_id){ return(0);}

void ue_decode_p(module_id_t module_idP, int CC_id, frame_t frame,
		 uint8_t CH_index, void *pdu, uint16_t len){}

void ue_decode_si(module_id_t module_idP, int CC_id, frame_t frame,
		  uint8_t CH_index, void *pdu, uint16_t len){}

void ue_send_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frame,
		 sub_frame_t subframe, uint8_t * sdu, uint16_t sdu_len,
		 uint8_t CH_index){}

uint16_t
ue_process_rar(const module_id_t module_idP,
	       const int CC_id,
	       const frame_t frameP,
	       const rnti_t ra_rnti,
	       uint8_t * const dlsch_buffer,
	       rnti_t * const t_crnti,
	       const uint8_t preamble_index,
	       uint8_t * selected_rar_buffer){ return(0);}

void ue_send_mch_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
		     uint8_t * sdu, uint16_t sdu_len, uint8_t eNB_index,
		     uint8_t sync_area){}

int ue_query_mch(module_id_t Mod_id, uint8_t CC_id, uint32_t frame,
		 sub_frame_t subframe, uint8_t eNB_index,
		 uint8_t * sync_area, uint8_t * mcch_active){ return(0);}


int ue_query_mch_fembms(module_id_t Mod_id, uint8_t CC_id, uint32_t frame,
		 sub_frame_t subframe, uint8_t eNB_index,
		 uint8_t * sync_area, uint8_t * mcch_active){ return(0);}

void dl_phy_sync_success(module_id_t module_idP,
			 frame_t frameP,
			 unsigned char eNB_index, uint8_t first_sync){}

uint64_t from_nrarfcn(int nr_bandP, uint32_t dl_nrarfcn) { return(0);}

int32_t get_uldl_offset(int nr_bandP) { return(0);}

IF_Module_t *IF_Module_init(int Mod_id) { return(NULL);}

int8_t get_Po_NOMINAL_PUSCH(module_id_t module_idP, uint8_t CC_id) { return(0);}

int8_t get_deltaP_rampup(module_id_t module_idP, uint8_t CC_id) { return(0);}

void thread_top_init(char *thread_name,
                     int affinity,
                     uint64_t runtime,
                     uint64_t deadline,
                     uint64_t period) {}

int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) { return(0);}
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req) { return(0); }
int oai_nfapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req) { return(0);  }
int oai_nfapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req){ return(0);  }
int oai_nfapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req){ return(0);  }
int oai_nfapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req){ return(0);  }
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) { return(0);  }
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) { return(0);  }
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) { return(0);  }
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) { return(0);  }
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) { return(0);  }

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) { return(0); }

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }

static int step_log = 0;
const char log_first[] = "debug";
const char log_second[] = "full";
const char log_default[] = " ";

int config_get(paramdef_t *params,int numparams, char *prefix)
{
#define MAX_STRING_SIZE     (255)

  char *p_log;

  for (int i=0; i < numparams; i++)
  {
	switch(step_log)
	{
	  case 1:
	    p_log = log_first;
	    params[i].strptr = calloc(1, MAX_STRING_SIZE);
	    *(params[i].strptr) = p_log;
	    break;
	  case 2:
	    p_log = log_first;
	    params[i].strptr = calloc(1, MAX_STRING_SIZE);
	    *(params[i].strptr) = p_log;
	    break;
	  case 3:
	    params[i].uptr = calloc(1, sizeof(int));
	    *(params[i].uptr) = 0;
	    break;
	  default:
		p_log = log_default;
		params[i].strptr = calloc(1, MAX_STRING_SIZE);
		*(params[i].strptr) = p_log;
		break;
	}
  }

  step_log++;

  return(0);
}

unsigned int taus(void) { return (0); }

typedef int loader_shlibfunc_t;

int load_module_shlib(char *modname,loader_shlibfunc_t *farray, int numf) { return(0); }

void * get_shlibmodule_fptr(char *modname, char *fname) { return(NULL) ; }

/*void exit_fun (const char *s) {
  VOID_PARAMETER s;
  undefined_function(__FUNCTION__);
}*/

uint32_t ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
		          uint8_t eNB_id, rnti_t rnti, sub_frame_t subframe){
	uint32_t value = 0;
	return(value);
}
