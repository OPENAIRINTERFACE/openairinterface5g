
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

uint32_t ue_get_SR(module_id_t module_idP, int CC_id, frame_t frameP,
		   uint8_t eNB_id, rnti_t rnti, sub_frame_t subframe){ return(0);}

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

void ue_decode_si_mbms(module_id_t module_idP, int CC_id, frame_t frame,
		  uint8_t CH_index, void *pdu, uint16_t len){}

void ue_send_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frame,
		 sub_frame_t subframe, uint8_t * sdu, uint16_t sdu_len,
		 uint8_t CH_index){}

SLSS_t *ue_get_slss(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe) {return(NULL);}

SLDCH_t *ue_get_sldch(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe) {return(NULL);}

SLSCH_t *ue_get_slsch(module_id_t module_idP, int CC_id,frame_t frameP, sub_frame_t subframe) {return(NULL);}

void multicast_link_write_sock(int groupP, char *dataP, uint32_t sizeP) {}

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

void dl_phy_sync_success(module_id_t module_idP,
			 frame_t frameP,
			 unsigned char eNB_index, uint8_t first_sync){}

uint32_t from_earfcn(int eutra_bandP, uint32_t dl_earfcn) { return(0);}

int32_t get_uldl_offset(int eutra_bandP) { return(0);}

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

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) { return(0); }
int oai_nfapi_ue_release_req(nfapi_ue_release_request_t *release_req){ return(0); }
int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }
