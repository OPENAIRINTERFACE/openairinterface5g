
//#include "openair1/PHY/defs.h"
//#include "openair2/PHY_INTERFACE/IF_Module.h"
//#include "openair1/PHY/extern.h"
#include "openair2/LAYER2/MAC/mac_extern.h"
#include "openair2/LAYER2/MAC/mac.h"
#include "openair2/LAYER2/MAC/mac_proto.h"
//#include "openair2/LAYER2/MAC/vars.h"
#include "openair1/SCHED_UE/sched_UE.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
//#include "common/ran_context.h"
#include "openair2/PHY_INTERFACE/phy_stub_UE.h"
#include "openair2/ENB_APP/L1_paramdef.h"
#include "openair2/ENB_APP/enb_paramdef.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"
#include "common/config/config_load_configmodule.h"
#include "common/config/config_userapi.h"

//#define DEADLINE_SCHEDULER 1


extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
extern int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);
void configure_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port);


UL_IND_t *UL_INFO = NULL;
nfapi_tx_request_pdu_t* tx_request_pdu_list = NULL;
nfapi_dl_config_request_t* dl_config_req = NULL;
nfapi_ul_config_request_t* ul_config_req = NULL;
nfapi_hi_dci0_request_t* hi_dci0_req = NULL;

//extern uint8_t nfapi_pnf;
//UL_IND_t *UL_INFO;
extern nfapi_tx_request_pdu_t* tx_request_pdu[1023][10][10];
//extern int timer_subframe;
//extern int timer_frame;

extern uint16_t sf_ahead;

void Msg1_transmitted(module_id_t module_idP,uint8_t CC_id,frame_t frameP, uint8_t eNB_id);
void Msg3_transmitted(module_id_t module_idP,uint8_t CC_id,frame_t frameP, uint8_t eNB_id);



void fill_rx_indication_UE_MAC(module_id_t Mod_id,int frame,int subframe, UL_IND_t* UL_INFO, uint8_t *ulsch_buffer, uint16_t buflen, uint16_t rnti, int index)
{
	  nfapi_rx_indication_pdu_t *pdu;

	  int timing_advance_update;


	  // pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
	  // change for mutiple UE's simulation.
	  pthread_mutex_lock(&fill_ul_mutex.rx_mutex);


	  UL_INFO->rx_ind.sfn_sf                    = frame<<4| subframe;
	  UL_INFO->rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
	  UL_INFO->rx_ind.vendor_extension		     = ul_config_req->vendor_extension;


	  pdu                                    = &UL_INFO->rx_ind.rx_indication_body.rx_pdu_list[UL_INFO->rx_ind.rx_indication_body.number_of_pdus];
	  //pdu                                    = &UL_INFO->rx_ind.rx_indication_body.rx_pdu_list[index];

	  //  pdu->rx_ue_information.handle          = eNB->ulsch[UE_id]->handle;
	  pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
	  pdu->rx_ue_information.rnti            = rnti;
	  pdu->rx_indication_rel8.tl.tag         = NFAPI_RX_INDICATION_REL8_TAG;
	  //pdu->rx_indication_rel8.length         = eNB->ulsch[UE_id]->harq_processes[harq_pid]->TBS>>3;
	  pdu->rx_indication_rel8.length         = buflen;
	  pdu->rx_indication_rel8.offset         = 1;   // DJP - I dont understand - but broken unless 1 ????  0;  // filled in at the end of the UL_INFO formation

	  // ulsch_buffer is necessary to keep its value.
	  //pdu->data                              = ulsch_buffer;
	  pdu->data = malloc(buflen);
	  memcpy(pdu->data,ulsch_buffer,buflen);
	  // estimate timing advance for MAC
	  //sync_pos                               = lte_est_timing_advance_pusch(eNB,UE_id);
	  timing_advance_update                  = 0;  // Don't know what to put here
	  pdu->rx_indication_rel8.timing_advance = timing_advance_update;

		  int SNRtimes10 = 640;

	  if      (SNRtimes10 < -640) pdu->rx_indication_rel8.ul_cqi=0;
	  else if (SNRtimes10 >  635) pdu->rx_indication_rel8.ul_cqi=255;
	  else                        pdu->rx_indication_rel8.ul_cqi=(640+SNRtimes10)/5;


	  UL_INFO->rx_ind.rx_indication_body.number_of_pdus++;
	  UL_INFO->rx_ind.sfn_sf = frame<<4 | subframe;
	  // pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
	  // change for mutiple UE's simulation.
	  pthread_mutex_unlock(&fill_ul_mutex.rx_mutex);


}

void fill_sr_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint16_t rnti) {


  // change for mutiple UE's simulation.
  //pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_lock(&fill_ul_mutex.sr_mutex);

  nfapi_sr_indication_t       *sr_ind = &UL_INFO->sr_ind;
  nfapi_sr_indication_body_t  *sr_ind_body =    &sr_ind->sr_indication_body;
  nfapi_sr_indication_pdu_t *pdu =   &sr_ind_body->sr_pdu_list[sr_ind_body->number_of_srs];
  UL_INFO->sr_ind.vendor_extension		     = ul_config_req->vendor_extension;

  //nfapi_sr_indication_pdu_t *pdu =   &UL_INFO->sr_ind.sr_indication_body.sr_pdu_list[UL_INFO->rx_ind.rx_indication_body.number_of_pdus];

  sr_ind->sfn_sf = frame<<4|subframe;
  sr_ind->header.message_id = NFAPI_RX_SR_INDICATION;

  sr_ind_body->tl.tag = NFAPI_SR_INDICATION_BODY_TAG;

  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = rnti; //UE_mac_inst[Mod_id].crnti


  // dependency from PHY not sure how to substitute this. Should we hardcode it?
  //int SNRtimes10 = dB_fixed_times10(stat) - 200;//(10*eNB->measurements.n0_power_dB[0]);
  int SNRtimes10 = 640;

  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;


  if      (SNRtimes10 < -640) pdu->ul_cqi_information.ul_cqi=0;
  else if (SNRtimes10 >  635) pdu->ul_cqi_information.ul_cqi=255;
  else                        pdu->ul_cqi_information.ul_cqi=(640+SNRtimes10)/5;
  pdu->ul_cqi_information.channel = 0;

  //UL_INFO->rx_ind.rx_indication_body.number_of_pdus++;
  sr_ind_body->number_of_srs++;
  // change for mutiple UE's simulation.
  // pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_unlock(&fill_ul_mutex.sr_mutex);
}


void fill_crc_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t crc_flag, int index, uint16_t rnti) {

  // change for mutiple UE's simulation.
  //pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_lock(&fill_ul_mutex.crc_mutex);

  // REMEMBER HAVE EXCHANGED THE FOLLOWING TWO LINES HERE!
  nfapi_crc_indication_pdu_t *pdu =   &UL_INFO->crc_ind.crc_indication_body.crc_pdu_list[UL_INFO->crc_ind.crc_indication_body.number_of_crcs];

  UL_INFO->crc_ind.sfn_sf                    = frame<<4| subframe;
  UL_INFO->crc_ind.vendor_extension		     = ul_config_req->vendor_extension;
  UL_INFO->crc_ind.header.message_id              = NFAPI_CRC_INDICATION;
  UL_INFO->crc_ind.crc_indication_body.tl.tag = NFAPI_CRC_INDICATION_BODY_TAG;

  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;

  //pdu->rx_ue_information.rnti                         = UE_mac_inst[Mod_id].crnti;
  pdu->rx_ue_information.rnti                         = rnti;
  pdu->crc_indication_rel8.tl.tag                     = NFAPI_CRC_INDICATION_REL8_TAG;
  pdu->crc_indication_rel8.crc_flag                   = crc_flag;

  UL_INFO->crc_ind.crc_indication_body.number_of_crcs++;

  LOG_D(PHY, "%s() rnti:%04x pdus:%d\n", __FUNCTION__, pdu->rx_ue_information.rnti, UL_INFO->crc_ind.crc_indication_body.number_of_crcs);

  // change for mutiple UE's simulation.
  // pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_unlock(&fill_ul_mutex.crc_mutex);
}

void fill_rach_indication_UE_MAC(int Mod_id,int frame,int subframe, UL_IND_t *UL_INFO, uint8_t ra_PreambleIndex, uint16_t ra_RNTI) {

	LOG_D(MAC, "fill_rach_indication_UE_MAC 1 \n");

	// change for mutiple UE's simulation.
	// pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
	pthread_mutex_lock(&fill_ul_mutex.rach_mutex);

	// memory allocation and free memory of UL_INFO are done in UE_phy_stub_single_thread_rxn_txnp4.
	// UL_INFO = (UL_IND_t*)malloc(sizeof(UL_IND_t));

	    UL_INFO->rach_ind.rach_indication_body.number_of_preambles                 = 1;

	    //eNB->UL_INFO.rach_ind.preamble_list                       = &eNB->preamble_list[0];
	    UL_INFO->rach_ind.header.message_id                         = NFAPI_RACH_INDICATION;
	    UL_INFO->rach_ind.sfn_sf                                    = frame<<4 | subframe;
	    UL_INFO->rach_ind.vendor_extension							= NULL;

	    UL_INFO->rach_ind.rach_indication_body.tl.tag                              = NFAPI_RACH_INDICATION_BODY_TAG;


	    UL_INFO->rach_ind.rach_indication_body.preamble_list = (nfapi_preamble_pdu_t*)malloc(UL_INFO->rach_ind.rach_indication_body.number_of_preambles*sizeof(nfapi_preamble_pdu_t));
	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.tl.tag   		= NFAPI_PREAMBLE_REL8_TAG;
	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.timing_advance = 0; // Not sure about that

	    //The two following should get extracted from the call to get_prach_resources().
	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.preamble = ra_PreambleIndex;
	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.rnti 	  = ra_RNTI;
	    //UL_INFO->rach_ind.rach_indication_body.number_of_preambles++;


	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel13.rach_resource_type = 0;
	    UL_INFO->rach_ind.rach_indication_body.preamble_list[0].instance_length					 = 0;


	          LOG_E(PHY,"\n\n\n\nDJP - this needs to be sent to VNF **********************************************\n\n\n\n");
	          LOG_E(PHY,"UE Filling NFAPI indication for RACH : TA %d, Preamble %d, rnti %x, rach_resource_type %d\n",
	        	  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.timing_advance,
	        	  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.preamble,
	        	  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel8.rnti,
	        	  UL_INFO->rach_ind.rach_indication_body.preamble_list[0].preamble_rel13.rach_resource_type);

	          // This function is currently defined only in the nfapi-RU-RAU-split so we should call it when we merge
	          // with that branch.
	          oai_nfapi_rach_ind(&UL_INFO->rach_ind);
	          free(UL_INFO->rach_ind.rach_indication_body.preamble_list);

	         // memory allocation and free memory of UL_INFO are done in UE_phy_stub_single_thread_rxn_txnp4.
	          //free(UL_INFO);

	        //}
	      // change for mutiple UE's simulation.
	      // pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
	      pthread_mutex_unlock(&fill_ul_mutex.rach_mutex);

}

void fill_ulsch_cqi_indication_UE_MAC(int Mod_id, uint16_t frame,uint8_t subframe, UL_IND_t *UL_INFO, uint16_t rnti) {

	// change for mutiple UE's simulation.
	//pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
	pthread_mutex_lock(&fill_ul_mutex.cqi_mutex);
	nfapi_cqi_indication_pdu_t *pdu         = &UL_INFO->cqi_ind.cqi_indication_body.cqi_pdu_list[UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis];
	nfapi_cqi_indication_raw_pdu_t *raw_pdu = &UL_INFO->cqi_ind.cqi_indication_body.cqi_raw_pdu_list[UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis];

	pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
	pdu->rx_ue_information.rnti = rnti;
	// Since we assume that CRC flag is always 0 (ACK) I guess that data_offset should always be 0.
	pdu->cqi_indication_rel9.data_offset = 0;

	// by default set O to rank 1 value
	//pdu->cqi_indication_rel9.length = (ulsch_harq->Or1>>3) + ((ulsch_harq->Or1&7) > 0 ? 1 : 0);
	//  Not useful field for our case
	pdu->cqi_indication_rel9.tl.tag = NFAPI_CQI_INDICATION_REL9_TAG;
	pdu->cqi_indication_rel9.length = 0;
	pdu->cqi_indication_rel9.ri[0]  = 0;


	pdu->cqi_indication_rel9.timing_advance = 0;
  pdu->cqi_indication_rel9.number_of_cc_reported = 1;
  pdu->ul_cqi_information.channel = 1; // PUSCH

  // Not sure how to substitute this. This should be the actual CQI value? So can
  // we hardcode it to a specific value?
  //memcpy((void*)raw_pdu->pdu,ulsch_harq->o,pdu->cqi_indication_rel9.length);
  raw_pdu->pdu[0] = 7;



  UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis++;
  // change for mutiple UE's simulation.
  //pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_unlock(&fill_ul_mutex.cqi_mutex);
}

void fill_ulsch_harq_indication_UE_MAC(int Mod_id, int frame,int subframe, UL_IND_t *UL_INFO, nfapi_ul_config_ulsch_harq_information *harq_information, uint16_t rnti)
{

  // change for mutiple UE's simulation.
  //pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_lock(&fill_ul_mutex.harq_mutex);

  nfapi_harq_indication_pdu_t *pdu =   &UL_INFO->harq_ind.harq_indication_body.harq_pdu_list[UL_INFO->harq_ind.harq_indication_body.number_of_harqs];
  int i;

  UL_INFO->harq_ind.header.message_id = NFAPI_HARQ_INDICATION;
  UL_INFO->harq_ind.sfn_sf = frame<<4|subframe;
  UL_INFO->harq_ind.vendor_extension		     = ul_config_req->vendor_extension;

  UL_INFO->harq_ind.harq_indication_body.tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;

  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti                         = rnti;

  // For now we consider only FDD
  //if (eNB->frame_parms.frame_type == FDD) {
    pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
    pdu->harq_indication_fdd_rel13.mode = 0;
    pdu->harq_indication_fdd_rel13.number_of_ack_nack = harq_information->harq_information_rel10.harq_size;

    // Could this be wrong? Is the number_of_ack_nack field equivalent to O_ACK?
    //pdu->harq_indication_fdd_rel13.number_of_ack_nack = ulsch_harq->O_ACK;

    for (i=0;i<harq_information->harq_information_rel10.harq_size;i++) {

      pdu->harq_indication_fdd_rel13.harq_tb_n[i] = 1; // Assuming always an ACK (No NACK or DTX)

    }

  UL_INFO->harq_ind.harq_indication_body.number_of_harqs++;
  // change for mutiple UE's simulation.
  //pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_unlock(&fill_ul_mutex.harq_mutex);}


void fill_uci_harq_indication_UE_MAC(int Mod_id,
			      int frame,
			      int subframe,
			      UL_IND_t *UL_INFO,
			      nfapi_ul_config_harq_information *harq_information,
			      uint16_t rnti
			      /*uint8_t tdd_mapping_mode,
			      uint16_t tdd_multiplexing_mask*/) {


  // change for mutiple UE's simulation.
  //pthread_mutex_lock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_lock(&fill_ul_mutex.harq_mutex);

  nfapi_harq_indication_t *ind       = &UL_INFO->harq_ind;
  nfapi_harq_indication_body_t *body = &ind->harq_indication_body;
  nfapi_harq_indication_pdu_t *pdu =   &body->harq_pdu_list[UL_INFO->harq_ind.harq_indication_body.number_of_harqs];

  UL_INFO->harq_ind.vendor_extension		     = ul_config_req->vendor_extension;

  ind->sfn_sf = frame<<4|subframe;
  ind->header.message_id = NFAPI_HARQ_INDICATION;

  body->tl.tag = NFAPI_HARQ_INDICATION_BODY_TAG;
  pdu->rx_ue_information.tl.tag                       = NFAPI_RX_UE_INFORMATION_TAG;

  pdu->instance_length                                = 0; // don't know what to do with this
  //  pdu->rx_ue_information.handle                       = handle;
  pdu->rx_ue_information.rnti                         = rnti;

  pdu->ul_cqi_information.tl.tag = NFAPI_UL_CQI_INFORMATION_TAG;

  int SNRtimes10 = 640;


  if      (SNRtimes10 < -640) pdu->ul_cqi_information.ul_cqi=0;
  else if (SNRtimes10 >  635) pdu->ul_cqi_information.ul_cqi=255;
  else                        pdu->ul_cqi_information.ul_cqi=(640+SNRtimes10)/5;
  pdu->ul_cqi_information.channel = 0;
  if(harq_information->harq_information_rel9_fdd.tl.tag == NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG){
      if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0) &&
          (harq_information->harq_information_rel9_fdd.harq_size == 1)) {

      pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 1;

      //AssertFatal(harq_ack[0] == 1 || harq_ack[0] == 2 || harq_ack[0] == 4, "harq_ack[0] is %d, should be 1,2 or 4\n",harq_ack[0]);
      pdu->harq_indication_fdd_rel13.harq_tb_n[0] = 1; // Assuming always an ACK (No NACK or DTX)


    }
    else if ((harq_information->harq_information_rel9_fdd.ack_nack_mode == 0) &&
                 (harq_information->harq_information_rel9_fdd.harq_size == 2)) {
      pdu->harq_indication_fdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_FDD_REL13_TAG;
      pdu->harq_indication_fdd_rel13.mode = 0;
      pdu->harq_indication_fdd_rel13.number_of_ack_nack = 2;
      pdu->harq_indication_fdd_rel13.harq_tb_n[0] = 1; // Assuming always an ACK (No NACK or DTX)
      pdu->harq_indication_fdd_rel13.harq_tb_n[1] = 1; // Assuming always an ACK (No NACK or DTX)

    }
  }else if(harq_information->harq_information_rel10_tdd.tl.tag == NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG ){
      if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 0) &&
          (harq_information->harq_information_rel10_tdd.harq_size == 1)) {
        pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
        pdu->harq_indication_tdd_rel13.mode = 0;
        pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
        pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;

      }  else if ((harq_information->harq_information_rel10_tdd.ack_nack_mode == 1) &&
                  (harq_information->harq_information_rel10_tdd.harq_size == 2)) {
        pdu->harq_indication_tdd_rel13.tl.tag = NFAPI_HARQ_INDICATION_TDD_REL13_TAG;
        pdu->harq_indication_tdd_rel13.mode = 0;
        pdu->harq_indication_tdd_rel13.number_of_ack_nack = 1;
        pdu->harq_indication_tdd_rel13.harq_data[0].bundling.value_0 = 1;
        pdu->harq_indication_tdd_rel13.harq_data[1].bundling.value_0 = 1;

      }
  } else AssertFatal(1==0,"only format 1a/b for now, received \n");


  UL_INFO->harq_ind.harq_indication_body.number_of_harqs++;
  LOG_D(PHY,"Incremented eNB->UL_INFO.harq_ind.number_of_harqs:%d\n", UL_INFO->harq_ind.harq_indication_body.number_of_harqs);
  // change for mutiple UE's simulation.
  //pthread_mutex_unlock(&UE_mac_inst[Mod_id].UL_INFO_mutex);
  pthread_mutex_unlock(&fill_ul_mutex.harq_mutex);

}


void handle_nfapi_ul_pdu_UE_MAC(module_id_t Mod_id,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,uint8_t subframe,uint8_t srs_present, int index)
{

  if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE) {
    LOG_D(PHY,"Applying UL config for UE, rnti %x for frame %d, subframe %d\n",
         (ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8).rnti,frame,subframe);
    uint8_t ulsch_buffer[5477] __attribute__ ((aligned(32)));
    uint16_t buflen = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size;

    uint16_t rnti = ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti;
    uint8_t access_mode=SCHEDULED_ACCESS;
    if(buflen>0){
    	if(UE_mac_inst[Mod_id].first_ULSCH_Tx == 1){ // Msg3 case
    		LOG_D(MAC, "handle_nfapi_ul_pdu_UE_MAC 2.2, Mod_id:%d, SFN/SF: %d/%d \n", Mod_id, frame, subframe);
    		fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
    		fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, UE_mac_inst[Mod_id].RA_prach_resources.Msg3,buflen, rnti, index);
    		Msg3_transmitted(Mod_id, 0, frame, 0);
    		//  Modification
    		UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
    		UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;

    		// This should be done after the reception of the respective hi_dci0
    		//UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
    	}
    	else {
    		ue_get_sdu( Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
    		fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
    		fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_buffer,buflen, rnti, index);
    	}
    }
  }

  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE) {

	  //AssertFatal((UE_id = find_ulsch(ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti,eNB,SEARCH_EXIST_OR_FREE))>=0,
    //            "No available UE ULSCH for rnti %x\n",ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti);
	  uint8_t ulsch_buffer[5477] __attribute__ ((aligned(32)));
	  uint16_t buflen = ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.size;
	  nfapi_ul_config_ulsch_harq_information *ulsch_harq_information = &ul_config_pdu->ulsch_harq_pdu.harq_information;
	  uint16_t rnti = ul_config_pdu->ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
	  uint8_t access_mode=SCHEDULED_ACCESS;
	  if(buflen>0){
		  if(UE_mac_inst[Mod_id].first_ULSCH_Tx == 1){ // Msg3 case
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, UE_mac_inst[Mod_id].RA_prach_resources.Msg3,buflen, rnti, index);
			  Msg3_transmitted(Mod_id, 0, frame, 0);
			  //UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
			  // Modification
			  UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
			  UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
		  }
		  else {
			  ue_get_sdu( Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_buffer,buflen, rnti, index);
		  }

	  }
	  if(ulsch_harq_information!=NULL)
		  fill_ulsch_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti);

  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE) {
	 uint8_t ulsch_buffer[5477] __attribute__ ((aligned(32)));
	  uint16_t buflen = ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.size;

	  uint16_t rnti = ul_config_pdu->ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
	  uint8_t access_mode=SCHEDULED_ACCESS;
	  if(buflen>0){
		  if(UE_mac_inst[Mod_id].first_ULSCH_Tx == 1){ // Msg3 case
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, UE_mac_inst[Mod_id].RA_prach_resources.Msg3,buflen, rnti, index);
			  Msg3_transmitted(Mod_id, 0, frame, 0);
			  //UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
			  // Modification
			  UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
			  UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
		  }
		  else {
			  ue_get_sdu( Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_buffer,buflen, rnti, index);
		  }
	  }
	  fill_ulsch_cqi_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti);

  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE) {

	  uint8_t ulsch_buffer[5477] __attribute__ ((aligned(32)));
	  uint16_t buflen = ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.size;
	  nfapi_ul_config_ulsch_harq_information *ulsch_harq_information = &ul_config_pdu->ulsch_cqi_harq_ri_pdu.harq_information;

	  uint16_t rnti = ul_config_pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti;
	  uint8_t access_mode=SCHEDULED_ACCESS;
	  if(buflen>0){
		  if(UE_mac_inst[Mod_id].first_ULSCH_Tx == 1){ // Msg3 case
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, UE_mac_inst[Mod_id].RA_prach_resources.Msg3,buflen, rnti, index);
			  Msg3_transmitted(Mod_id, 0, frame, 0);
			  //UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
			  // Modification
			  UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
			  UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
		  }
		  else {
			  ue_get_sdu( Mod_id, 0, frame, subframe, 0, ulsch_buffer, buflen, &access_mode);
			  fill_crc_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, 0, index, rnti);
			  fill_rx_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_buffer,buflen, rnti, index);
		  }
	  }

	  if(ulsch_harq_information!=NULL)
		  fill_ulsch_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, ulsch_harq_information, rnti);
	  fill_ulsch_cqi_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti);

  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) {

	  uint16_t rnti = ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti;

	  nfapi_ul_config_harq_information *ulsch_harq_information = &ul_config_pdu->uci_harq_pdu.harq_information;
	  if(ulsch_harq_information != NULL)
		  fill_uci_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO,ulsch_harq_information, rnti);
  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE not handled yet\n");
  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE not handled yet\n");
  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE) {
    AssertFatal(1==0,"NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE not handled yet\n");
  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE) {

	  uint16_t rnti = ul_config_pdu->uci_sr_pdu.ue_information.ue_information_rel8.rnti;

	  if (ue_get_SR(Mod_id ,0,frame, 0, rnti, subframe))
		  fill_sr_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO, rnti);

  }
  else if (ul_config_pdu->pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE) {
    //AssertFatal((UE_id = find_uci(rel8->rnti,proc->frame_tx,proc->subframe_tx,eNB,SEARCH_EXIST_OR_FREE))>=0,
    //            "No available UE UCI for rnti %x\n",ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti);

	  uint16_t rnti = ul_config_pdu->uci_sr_harq_pdu.ue_information.ue_information_rel8.rnti;

	  // We fill the sr_indication only if ue_get_sr() would normally instruct PHY to send a SR.
	  if (ue_get_SR(Mod_id ,0,frame, 0, rnti, subframe))
		  fill_sr_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO,rnti);

	  nfapi_ul_config_harq_information *ulsch_harq_information = &ul_config_pdu->uci_sr_harq_pdu.harq_information;
	  if (ulsch_harq_information != NULL)
		  fill_uci_harq_indication_UE_MAC(Mod_id, frame, subframe, UL_INFO,ulsch_harq_information, rnti);

  }

}






int ul_config_req_UE_MAC(nfapi_ul_config_request_t* req, int timer_frame, int timer_subframe, module_id_t Mod_id)
{
	//if (req!=NULL){ // && req->ul_config_request_body.ul_config_pdu_list !=NULL){
  LOG_D(PHY,"[PNF] UL_CONFIG_REQ %s() sfn_sf:%d pdu:%d rach_prach_frequency_resources:%d srs_present:%u\n",
      __FUNCTION__,
      NFAPI_SFNSF2DEC(req->sfn_sf),
      req->ul_config_request_body.number_of_pdus,
      req->ul_config_request_body.rach_prach_frequency_resources,
      req->ul_config_request_body.srs_present
      );

  int sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
  int sf = NFAPI_SFNSF2SF(req->sfn_sf);


  LOG_D(MAC, "ul_config_req_UE_MAC() TOTAL NUMBER OF UL_CONFIG PDUs: %d, SFN/SF: %d/%d \n", req->ul_config_request_body.number_of_pdus, timer_frame, timer_subframe);



  for (int i=0;i<req->ul_config_request_body.number_of_pdus;i++)
  {

    if (
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].ulsch_pdu.ulsch_pdu_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].ulsch_harq_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].ulsch_cqi_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].ulsch_cqi_harq_ri_pdu.ulsch_pdu.ulsch_pdu_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].uci_cqi_harq_pdu.ue_information.ue_information_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].uci_sr_pdu.ue_information.ue_information_rel8.rnti == UE_mac_inst[Mod_id].crnti) ||
    		(req->ul_config_request_body.ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE && req->ul_config_request_body.ul_config_pdu_list[i].uci_cqi_sr_harq_pdu.ue_information.ue_information_rel8.rnti == UE_mac_inst[Mod_id].crnti)
       )
    {

      handle_nfapi_ul_pdu_UE_MAC(Mod_id,&req->ul_config_request_body.ul_config_pdu_list[i],sfn,sf,req->ul_config_request_body.srs_present, i);

    }
    else
    {
      //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() PDU:%i UNKNOWN type :%d\n", __FUNCTION__, i, ul_config_pdu_list[i].pdu_type);
    }
  }

//	}

  return 0;
}



int tx_req_UE_MAC(nfapi_tx_request_t* req)
{

  LOG_D(PHY,"%s() SFN/SF:%d/%d PDUs:%d\n", __FUNCTION__, NFAPI_SFNSF2SFN(req->sfn_sf), NFAPI_SFNSF2SF(req->sfn_sf), req->tx_request_body.number_of_pdus);


    for (int i=0; i<req->tx_request_body.number_of_pdus; i++)
    {
      LOG_D(PHY,"%s() SFN/SF:%d/%d number_of_pdus:%d [PDU:%d] pdu_length:%d pdu_index:%d num_segments:%d\n",
          __FUNCTION__,
          NFAPI_SFNSF2SFN(req->sfn_sf), NFAPI_SFNSF2SF(req->sfn_sf),
          req->tx_request_body.number_of_pdus,
          i,
          req->tx_request_body.tx_pdu_list[i].pdu_length,
          req->tx_request_body.tx_pdu_list[i].pdu_index,
          req->tx_request_body.tx_pdu_list[i].num_segments
          );

    }

  return 0;
}


int dl_config_req_UE_MAC(nfapi_dl_config_request_t* req, module_id_t Mod_id) //, nfapi_tx_request_pdu_t* tx_request_pdu_list)
{
	//if (req!=NULL && tx_request_pdu_list!=NULL){
  int sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
  int sf = NFAPI_SFNSF2SF(req->sfn_sf);
  //Mod_id = 0; // Currently static (only for one UE) but this should change.

  /*struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  L1_rxtx_proc_t *proc = &eNB->proc.L1_proc;*/
  nfapi_dl_config_request_pdu_t* dl_config_pdu_list = req->dl_config_request_body.dl_config_pdu_list;
  nfapi_dl_config_request_pdu_t *dl_config_pdu_tmp;

  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() TX:%d/%d RX:%d/%d sfn_sf:%d DCI:%d PDU:%d\n", __FUNCTION__, proc->frame_tx, proc->subframe_tx, proc->frame_rx, proc->subframe_rx, NFAPI_SFNSF2DEC(req->sfn_sf), req->dl_config_request_body.number_dci, req->dl_config_request_body.number_pdu);



  //LOG_D(PHY,"NFAPI: Sched_INFO:SFN/SF:%d%d dl_pdu:%d tx_req:%d hi_dci0:%d ul_cfg:%d num_pdcch_symbols:%d\n",
  //	frame,subframe,number_dl_pdu,TX_req->tx_request_body.number_of_pdus,number_hi_dci0_pdu,number_ul_pdu, eNB->pdcch_vars[subframe&1].num_pdcch_symbols);

  for (int i=0;i<req->dl_config_request_body.number_pdu;i++)
  {
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() sfn/sf:%d PDU[%d] size:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size);
	  //LOG_E(MAC, "dl_config_req_UE_MAC 2 Received real ones: sfn/sf:%d.%d PDU[%d] size:%d\n", sfn, sf, i, dl_config_pdu_list[i].pdu_size);

    if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
    {
		if (dl_config_pdu_list[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type == 1) {
			// C-RNTI (Normal DLSCH case)
			dl_config_pdu_tmp = &dl_config_pdu_list[i+1];
			if (dl_config_pdu_tmp->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE && UE_mac_inst[Mod_id].crnti == dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.rnti){

				/*nfapi_tx_request_pdu_t *ptr = tx_request_pdu_list;
				ptr += dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index*(sizeof(nfapi_tx_request_pdu_t));
				nfapi_tx_request_pdu_t temp;
				memset(&temp, 0, sizeof(nfapi_tx_request_pdu_t));
				//if (!memcmp(&temp, ptr, sizeof(temp)) ...
				if( *(char*)ptr != 0){
				//if(tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data!= NULL && tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_length >0){
				*/

				// to avoid unexpected error , add check pdu_index is more than 0.
				if((dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index >= 0) &&(dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index <= tx_req_num_elems -1)){
				//if(tx_request_pdu_list + dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index!= NULL){

					LOG_E(MAC, "dl_config_req_UE_MAC 2 Received data: sfn/sf:%d PDU[%d] size:%d, TX_PDU index: %d, tx_req_num_elems: %d \n", NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size, dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index, tx_req_num_elems);

					ue_send_sdu(Mod_id, 0, sfn, sf,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_length,
							0);
					i++;
				}
				else{
					LOG_E(MAC,"dl_config_req_UE_MAC 2: Problem with receiving data: sfn/sf:%d PDU[%d] size:%d, TX_PDU index: %d\n", NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size, dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index);
					i++;
				}
			}
			else {
				LOG_E(MAC,"[UE %d] Frame %d, subframe %d : DLSCH PDU from NFAPI not for this UE \n",Mod_id, sfn,sf);
				i++;
			}
		}
		else if (dl_config_pdu_list[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type == 2) {
			dl_config_pdu_tmp = &dl_config_pdu_list[i+1];
			if(dl_config_pdu_tmp->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE && dl_config_pdu_list[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti == 0xFFFF && UE_mac_inst[Mod_id].UE_mode[0] != NOT_SYNCHED){ //&& UE_mac_inst[Mod_id].UE_mode[0] != NOT_SYNCHED

				if(dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index <= tx_req_num_elems -1){
				//if(tx_request_pdu_list + dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index!= NULL){
					ue_decode_si(Mod_id, 0, sfn, 0,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_length);
					i++;
				}
				else{
					LOG_E(MAC,"dl_config_req_UE_MAC 3: Problem with receiving SI: sfn/sf:%d PDU[%d] size:%d, TX_PDU index: %d\n", NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size, dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index);
					i++;
				}
			}
			else if(dl_config_pdu_tmp->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE && dl_config_pdu_list[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti == 0xFFFE){
				// P_RNTI case
				//pdu = Tx_req->tx_request_body.tx_pdu_list[dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data;

				if (dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index <= tx_req_num_elems -1){
				//if(tx_request_pdu_list + dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index!= NULL){
					ue_decode_p(Mod_id, 0, sfn, 0,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_length);
					i++;
				}
				else{
					LOG_E(MAC,"dl_config_req_UE_MAC: Problem with receiving Paging: sfn/sf:%d PDU[%d] size:%d, TX_PDU index: %d\n", NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size, dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index);
					i++;
				}
			}
			else if(dl_config_pdu_tmp->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE) {
				// RA-RNTI case
				LOG_E(MAC,"dl_config_req_UE_MAC 4 Received RAR? \n");
				// RNTI parameter not actually used. Provided only to comply with existing function definition.
				// Not sure about parameters to fill the preamble index.
				//rnti_t c_rnti = UE_mac_inst[Mod_id].crnti;
				rnti_t ra_rnti = UE_mac_inst[Mod_id].RA_prach_resources.ra_RNTI;
				if ((UE_mac_inst[Mod_id].UE_mode[0] != PUSCH) &&
				  (UE_mac_inst[Mod_id].RA_prach_resources.Msg3!=NULL) && (ra_rnti== dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.rnti) &&
				  //(tx_request_pdu_list + dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index!= NULL)) {
				  (dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index <= tx_req_num_elems -1)) {
					LOG_E(MAC,"dl_config_req_UE_MAC 5 Received RAR, PreambleIndex: %d \n", UE_mac_inst[Mod_id].RA_prach_resources.ra_PreambleIndex);
					ue_process_rar(Mod_id, 0, sfn,
							ra_rnti, //RA-RNTI
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data,
							&dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.rnti, //t-crnti
							UE_mac_inst[Mod_id].RA_prach_resources.ra_PreambleIndex,
							tx_request_pdu_list[dl_config_pdu_tmp->dlsch_pdu.dlsch_pdu_rel8.pdu_index].segments[0].segment_data);
					UE_mac_inst[Mod_id].UE_mode[0] = RA_RESPONSE;
					UE_mac_inst[Mod_id].first_ULSCH_Tx = 1; //Expecting an UL_CONFIG_ULSCH_PDU to enable Msg3 Txon (first ULSCH Txon for the UE)
				}
				i++;
			}
			else {
				LOG_E(MAC,"[UE %d] Frame %d, subframe %d : Cannot extract DLSCH PDU from NFAPI 2\n",Mod_id, sfn, sf);
				i++;
			}

		}
    }
    else if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_BCH_PDU_TYPE)
    {
    	// BCH case
    	// Last parameter is 1 if first time synchronization and zero otherwise. Not sure which value to put
    	// for our case.
    	//LOG_E(MAC,"dl_config_req_UE_MAC 4 Received MIB: sfn/sf: %d.%d \n", sfn, sf);
    	if(UE_mac_inst[Mod_id].UE_mode[0] == NOT_SYNCHED){
    		dl_phy_sync_success(Mod_id,sfn,0, 1);
    		LOG_E(MAC,"dl_config_req_UE_MAC 5 Received MIB: UE_mode: %d, sfn/sf: %d.%d\n", UE_mac_inst[Mod_id].UE_mode[0], sfn, sf);
    		UE_mac_inst[Mod_id].UE_mode[0]=PRACH;
    	}
    	else
    		dl_phy_sync_success(Mod_id,sfn,0, 0);

    }

    else
    {
      //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() UNKNOWN:%d\n", __FUNCTION__, dl_config_pdu_list[i].pdu_type);
    }
  }

  return 0;

}


int hi_dci0_req_UE_MAC(nfapi_hi_dci0_request_t* req, module_id_t Mod_id)
{
	if (req!=NULL && req->hi_dci0_request_body.hi_dci0_pdu_list!=NULL){
  LOG_D(PHY,"[UE-PHY_STUB] hi dci0 request sfn_sf:%d number_of_dci:%d number_of_hi:%d\n", NFAPI_SFNSF2DEC(req->sfn_sf), req->hi_dci0_request_body.number_of_dci, req->hi_dci0_request_body.number_of_hi);



  for (int i=0; i<req->hi_dci0_request_body.number_of_dci + req->hi_dci0_request_body.number_of_hi; i++)
  {
    LOG_D(PHY,"[UE-PHY_STUB] HI_DCI0_REQ sfn_sf:%d PDU[%d]\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);

    if (req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE)
    {
      LOG_D(PHY,"[UE-PHY_STUB] HI_DCI0_REQ sfn_sf:%d PDU[%d] - NFAPI_HI_DCI0_DCI_PDU_TYPE not used \n", NFAPI_SFNSF2DEC(req->sfn_sf), i);

    }
    else if (req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type == NFAPI_HI_DCI0_HI_PDU_TYPE)
    {
      //LOG_I(MAC,"[UE-PHY_STUB] HI_DCI0_REQ sfn_sf:%d PDU[%d] - NFAPI_HI_DCI0_HI_PDU_TYPE\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);

      nfapi_hi_dci0_request_pdu_t *hi_dci0_req_pdu = &req->hi_dci0_request_body.hi_dci0_pdu_list[i];

      // This is meaningful only after ACKnowledging the first ULSCH Txon (i.e. Msg3)
      if(hi_dci0_req_pdu->hi_pdu.hi_pdu_rel8.hi_value == 1 && UE_mac_inst[Mod_id].first_ULSCH_Tx == 1){
    	  //LOG_I(MAC,"[UE-PHY_STUB] HI_DCI0_REQ 2 sfn_sf:%d PDU[%d] - NFAPI_HI_DCI0_HI_PDU_TYPE\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);
    	  //UE_mac_inst[Mod_id].UE_mode[0] = PUSCH;
    	  //UE_mac_inst[Mod_id].first_ULSCH_Tx = 0;
      }

    }
    else
    {
      LOG_E(PHY,"[UE-PHY_STUB] HI_DCI0_REQ sfn_sf:%d PDU[%d] - unknown pdu type:%d\n", NFAPI_SFNSF2DEC(req->sfn_sf), i, req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type);
    }
  }

  }


  return 0;
}





// The following set of memcpy functions should be getting called as callback functions from
// pnf_p7_subframe_ind.
int memcpy_dl_config_req (nfapi_pnf_p7_config_t* pnf_p7, nfapi_dl_config_request_t* req)
{

	//for (Mod_id=0; Mod_id<NB_UE_INST; Mod_id++){

	dl_config_req = (nfapi_dl_config_request_t*)malloc(sizeof(nfapi_dl_config_request_t));

	//UE_mac_inst[Mod_id].dl_config_req->header = req->header;
	dl_config_req->sfn_sf = req->sfn_sf;

	dl_config_req->vendor_extension = req->vendor_extension;

	dl_config_req->dl_config_request_body.number_dci = req->dl_config_request_body.number_dci;
	dl_config_req->dl_config_request_body.number_pdcch_ofdm_symbols = req->dl_config_request_body.number_pdcch_ofdm_symbols;
	dl_config_req->dl_config_request_body.number_pdsch_rnti = req->dl_config_request_body.number_pdsch_rnti;
	dl_config_req->dl_config_request_body.number_pdu = req->dl_config_request_body.number_pdu;

	dl_config_req->dl_config_request_body.tl.tag = req->dl_config_request_body.tl.tag;
	dl_config_req->dl_config_request_body.tl.length = req->dl_config_request_body.tl.length;

	dl_config_req->dl_config_request_body.dl_config_pdu_list = (nfapi_dl_config_request_pdu_t*) calloc(req->dl_config_request_body.number_pdu, sizeof(nfapi_dl_config_request_pdu_t));
	for(int i=0; i<dl_config_req->dl_config_request_body.number_pdu; i++) {
		dl_config_req->dl_config_request_body.dl_config_pdu_list[i] = req->dl_config_request_body.dl_config_pdu_list[i];
	}

	//}

	return 0;

}

int memcpy_ul_config_req (nfapi_pnf_p7_config_t* pnf_p7, nfapi_ul_config_request_t* req)
{

	//for (Mod_id=0; Mod_id<NB_UE_INST; Mod_id++){

		ul_config_req = (nfapi_ul_config_request_t*)malloc(sizeof(nfapi_ul_config_request_t));

	ul_config_req->sfn_sf = req->sfn_sf;
	ul_config_req->vendor_extension = req->vendor_extension;


	ul_config_req->ul_config_request_body.number_of_pdus = req->ul_config_request_body.number_of_pdus;
	ul_config_req->ul_config_request_body.rach_prach_frequency_resources = req->ul_config_request_body.rach_prach_frequency_resources;
	ul_config_req->ul_config_request_body.srs_present = req->ul_config_request_body.srs_present;

	ul_config_req->ul_config_request_body.tl.tag = req->ul_config_request_body.tl.tag;
	ul_config_req->ul_config_request_body.tl.length = req->ul_config_request_body.tl.length;

	//LOG_D(MAC, "memcpy_ul_config_req 1 #ofULPDUs: %d \n", UE_mac_inst[Mod_id].ul_config_req->ul_config_request_body.number_of_pdus); //req->ul_config_request_body.number_of_pdus);
	ul_config_req->ul_config_request_body.ul_config_pdu_list = (nfapi_ul_config_request_pdu_t*) malloc(req->ul_config_request_body.number_of_pdus*sizeof(nfapi_ul_config_request_pdu_t));
	for(int i=0; i<ul_config_req->ul_config_request_body.number_of_pdus; i++) {
			ul_config_req->ul_config_request_body.ul_config_pdu_list[i] = req->ul_config_request_body.ul_config_pdu_list[i];
		}
	//}

	return 0;
}




int memcpy_tx_req (nfapi_pnf_p7_config_t* pnf_p7, nfapi_tx_request_t* req)
{

	tx_req_num_elems = req->tx_request_body.number_of_pdus;
	tx_request_pdu_list = (nfapi_tx_request_pdu_t*) calloc(tx_req_num_elems, sizeof(nfapi_tx_request_pdu_t));
	for (int i=0; i<tx_req_num_elems; i++) {
		tx_request_pdu_list[i].num_segments = req->tx_request_body.tx_pdu_list[i].num_segments;
		tx_request_pdu_list[i].pdu_index = req->tx_request_body.tx_pdu_list[i].pdu_index;
		tx_request_pdu_list[i].pdu_length = req->tx_request_body.tx_pdu_list[i].pdu_length;
		for (int j=0; j<req->tx_request_body.tx_pdu_list[i].num_segments; j++){
			//*tx_request_pdu_list[i].segments[j].segment_data = *req->tx_request_body.tx_pdu_list[i].segments[j].segment_data;
			tx_request_pdu_list[i].segments[j].segment_length = req->tx_request_body.tx_pdu_list[i].segments[j].segment_length;
			if(tx_request_pdu_list[i].segments[j].segment_length > 0){
				tx_request_pdu_list[i].segments[j].segment_data = (uint8_t*)malloc(tx_request_pdu_list[i].segments[j].segment_length*sizeof (uint8_t));
			memcpy(tx_request_pdu_list[i].segments[j].segment_data, req->tx_request_body.tx_pdu_list[i].segments[j].segment_data, tx_request_pdu_list[i].segments[j].segment_length);
			}
		}

	}

	return 0;
}

int memcpy_hi_dci0_req (nfapi_pnf_p7_config_t* pnf_p7, nfapi_hi_dci0_request_t* req)
{

	//if(req!=0){

	//for (Mod_id=0; Mod_id<NB_UE_INST; Mod_id++){
	hi_dci0_req = (nfapi_hi_dci0_request_t*)malloc(sizeof(nfapi_hi_dci0_request_t));

	hi_dci0_req->sfn_sf = req->sfn_sf;
	hi_dci0_req->vendor_extension = req->vendor_extension;

	hi_dci0_req->hi_dci0_request_body.number_of_dci = req->hi_dci0_request_body.number_of_dci;
	hi_dci0_req->hi_dci0_request_body.number_of_hi = req->hi_dci0_request_body.number_of_hi;
	hi_dci0_req->hi_dci0_request_body.sfnsf = req->hi_dci0_request_body.sfnsf;

	//UE_mac_inst[Mod_id].hi_dci0_req->hi_dci0_request_body.tl = req->hi_dci0_request_body.tl;
	hi_dci0_req->hi_dci0_request_body.tl.tag = req->hi_dci0_request_body.tl.tag;
	hi_dci0_req->hi_dci0_request_body.tl.length = req->hi_dci0_request_body.tl.length;

	int total_pdus = hi_dci0_req->hi_dci0_request_body.number_of_dci + hi_dci0_req->hi_dci0_request_body.number_of_hi;

	hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list = (nfapi_hi_dci0_request_pdu_t*) malloc(total_pdus*sizeof(nfapi_hi_dci0_request_pdu_t));

	for(int i=0; i<total_pdus; i++){
		hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list[i] = req->hi_dci0_request_body.hi_dci0_pdu_list[i];
		//LOG_I(MAC, "Original hi_dci0 req. type:%d, Copy type: %d \n",req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type, UE_mac_inst[Mod_id].hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type);
	}

	//}
		return 0;
}



void UE_config_stub_pnf(void) {
  int               j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST,NULL,0};

  config_getlist( &L1_ParamList,L1_Params,sizeof(L1_Params)/sizeof(paramdef_t), NULL);
  if (L1_ParamList.numelt > 0) {
	  for (j=0; j<L1_ParamList.numelt; j++){
		  //nb_L1_CC = *(L1_ParamList.paramarray[j][L1_CC_IDX].uptr); // Number of component carriers is of no use for the
	                                                            // phy_stub mode UE pnf. Maybe we can completely skip it.

		  if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
			  sf_ahead = 4; // Need 4 subframe gap between RX and TX
			  }
		  // Right now that we have only one UE (thread) it is ok to put the eth_params in the UE_mac_inst.
		  // Later I think we have to change that to attribute eth_params to a global element for all the UEs.
		  else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
			  stub_eth_params.local_if_name            = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
			  stub_eth_params.my_addr                  = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
			  stub_eth_params.remote_addr              = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
			  stub_eth_params.my_portc                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
			  stub_eth_params.remote_portc             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
			  stub_eth_params.my_portd                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
			  stub_eth_params.remote_portd             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
			  stub_eth_params.transp_preference        = ETH_UDP_MODE;

			  sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2
			  //configure_nfapi_pnf(UE_mac_inst[0].eth_params_n.remote_addr, UE_mac_inst[0].eth_params_n.remote_portc, UE_mac_inst[0].eth_params_n.my_addr, UE_mac_inst[0].eth_params_n.my_portd, UE_mac_inst[0].eth_params_n.remote_portd);
			  configure_nfapi_pnf(stub_eth_params.remote_addr, stub_eth_params.remote_portc, stub_eth_params.my_addr, stub_eth_params.my_portd, stub_eth_params.remote_portd);
		  }
		  else { // other midhaul
		  }
	  }
  }
  else {

  }
}


/* Dummy functions*/

void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                                  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu)
{

}


void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                                 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu)
{

}


void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,
                             int frame, int subframe,
                             L1_rxtx_proc_t *proc,
                             nfapi_dl_config_request_pdu_t *dl_config_pdu)
{

}


void handle_nfapi_bch_pdu(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
                          nfapi_dl_config_request_pdu_t *dl_config_pdu,
                          uint8_t *sdu)
{

}


void handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,L1_rxtx_proc_t *proc,
                            nfapi_dl_config_request_pdu_t *dl_config_pdu,
                            uint8_t codeword_index,
                            uint8_t *sdu)
{

}


void handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,uint8_t subframe,uint8_t srs_present)
{

}

void phy_config_request(PHY_Config_t *phy_config) {
}

void phy_config_update_sib2_request(PHY_Config_t *phy_config) {
}
void phy_config_update_sib13_request(PHY_Config_t *phy_config) {
}

uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn) { return(0);}

int32_t get_uldl_offset(int eutra_bandP) { return(0);}

int l1_north_init_eNB(void) {
return 0;
}

void init_eNB_afterRU(void) {

}

