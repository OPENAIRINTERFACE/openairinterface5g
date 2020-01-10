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


#ifndef M2AP_MESSAGES_TYPES_H_
#define M2AP_MESSAGES_TYPES_H_

#include "s1ap_messages_types.h"
#include "LTE_PhysCellId.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define M2AP_MCE_SCTP_REQ(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mce_sctp_req

#define M2AP_REGISTER_ENB_REQ(mSGpTR)           (mSGpTR)->ittiMsg.m2ap_register_enb_req
//#define M2AP_HANDOVER_REQ(mSGpTR)               (mSGpTR)->ittiMsg.m2ap_handover_req
//#define M2AP_HANDOVER_REQ_ACK(mSGpTR)           (mSGpTR)->ittiMsg.m2ap_handover_req_ack
#define M2AP_REGISTER_ENB_CNF(mSGpTR)           (mSGpTR)->ittiMsg.m2ap_register_enb_cnf
#define M2AP_DEREGISTERED_ENB_IND(mSGpTR)       (mSGpTR)->ittiMsg.m2ap_deregistered_enb_ind
//#define M2AP_UE_CONTEXT_RELEASE(mSGpTR)         (mSGpTR)->ittiMsg.m2ap_ue_context_release
//#define M2AP_HANDOVER_CANCEL(mSGpTR)            (mSGpTR)->ittiMsg.m2ap_handover_cancel

#define M2AP_ENB_SCTP_REQ(mSGpTR)                   (mSGpTR)->ittiMsg.m2ap_enb_sctp_req
#define M2AP_SETUP_REQ(mSGpTR)                     (mSGpTR)->ittiMsg.m2ap_setup_req
#define M2AP_SETUP_RESP(mSGpTR)                    (mSGpTR)->ittiMsg.m2ap_setup_resp
#define M2AP_SETUP_FAILURE(mSGpTR)                 (mSGpTR)->ittiMsg.m2ap_setup_failure


#define M2AP_REGISTER_MCE_REQ(mSGpTR)           (mSGpTR)->ittiMsg.m2ap_register_mce_req



#define M2AP_MBMS_SCHEDULING_INFORMATION(mSGpTR) 	(mSGpTR)->ittiMsg.m2ap_mbms_scheduling_information
#define M2AP_MBMS_SCHEDULING_INFORMATION_RESP(mSGpTR) 	(mSGpTR)->ittiMsg.m2ap_mbms_scheduling_information_resp
#define M2AP_MBMS_SESSION_START_REQ(mSGpTR) 		(mSGpTR)->ittiMsg.m2ap_session_start_req
#define M2AP_MBMS_SESSION_START_RESP(mSGpTR) 		(mSGpTR)->ittiMsg.m2ap_session_start_resp
#define M2AP_MBMS_SESSION_START_FAILURE(mSGpTR) 	(mSGpTR)->ittiMsg.m2ap_session_start_failure
#define M2AP_MBMS_SESSION_STOP_REQ(mSGpTR) 		(mSGpTR)->ittiMsg.m2ap_session_stop_req
#define M2AP_MBMS_SESSION_STOP_RESP(mSGpTR) 		(mSGpTR)->ittiMsg.m2ap_session_stop_resp

#define M2AP_RESET(mSGpTR)				(mSGpTR)->ittiMsg.m2ap_reset
#define M2AP_ENB_CONFIGURATION_UPDATE(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_enb_configuration_update
#define M2AP_ENB_CONFIGURATION_UPDATE_ACK(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_enb_configuration_update_ack
#define M2AP_ENB_CONFIGURATION_UPDATE_FAILURE(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_enb_configuration_update_failure
#define M2AP_MCE_CONFIGURATION_UPDATE(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mce_configuration_update
#define M2AP_MCE_CONFIGURATION_UPDATE_ACK(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_mce_configuration_update_ack
#define M2AP_MCE_CONFIGURATION_UPDATE_FAILURE(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_mce_configuration_update_failure

#define M2AP_ERROR_INDICATION(mSGpTR)			(mSGpTR)->ittiMsg.m2ap_error_indication
#define M2AP_MBMS_SESSION_UPDATE_REQ(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mbms_session_update_req
#define M2AP_MBMS_SESSION_UPDATE_RESP(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mbms_session_update_resp
#define	M2AP_MBMS_SESSION_UPDATE_FAILURE(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_mbms_session_update_failure
#define M2AP_MBMS_SERVICE_COUNTING_REPORT(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_mbms_service_counting_report
#define M2AP_MBMS_OVERLOAD_NOTIFICATION(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mbms_overload_notification
#define M2AP_MBMS_SERVICE_COUNTING_REQ(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mbms_service_counting_req
#define M2AP_MBMS_SERVICE_COUNTING_RESP(mSGpTR)		(mSGpTR)->ittiMsg.m2ap_mbms_service_counting_resp
#define M2AP_MBMS_SERVICE_COUNTING_FAILURE(mSGpTR)	(mSGpTR)->ittiMsg.m2ap_mbms_service_counting_failure

#define M2AP_MAX_NB_ENB_IP_ADDRESS 2
#define M2AP_MAX_NB_MCE_IP_ADDRESS 2

#define M2AP_MAX_NB_CELLS 2



typedef struct m2ap_net_ip_address_s {
  unsigned ipv4:1;
  unsigned ipv6:1;
  char ipv4_address[16];
  char ipv6_address[46];
} m2ap_net_ip_address_t;

typedef struct m2ap_enb_setup_req_s {
   //
} m2ap_enb_setup_req_t;


typedef struct m2ap_setup_req_s {

  // Midhaul networking parameters

  /* Connexion id used between SCTP/M2AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* The eNB IP address to bind */
  m2ap_net_ip_address_t MCE_M2_ip_address;
  m2ap_net_ip_address_t ENB_M2_ip_address;

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  uint16_t default_sctp_stream_id;

  // M2_Setup_Req payload
  uint64_t eNB_id;
  char *eNB_name;

  uint64_t GlobalENB_ID;
  char * ENBname;

  uint16_t num_mbms_available;

  /* M2AP_MBSFN_SynchronisationArea_ID_t */
  long mbsfn_synchronization_area[M2AP_MAX_NB_CELLS];

  /* eCGI->eCGI.pLMN_Identity */
  uint16_t plmn_identity[M2AP_MAX_NB_CELLS];
  /* eCGI->eCGI.eUTRANcellIdentifier */
  uint16_t eutran_cell_identifier[M2AP_MAX_NB_CELLS];

  struct{
	  uint16_t mbsfn_sync_area;
  	  uint16_t mbms_service_area_list[8];
	  uint16_t num_mbms_service_area_list;
  }mbms_configuration_data_list[8];
  uint16_t num_mbms_configuration_data_list;


//
//  /* The type of the cell */
//  enum cell_type_e cell_type;
//
//  /// number of DU cells available
//  uint16_t num_cells_available; //0< num_cells_available <= 512;
//
//  // 
//  uint16_t num_mbms_available;
//
//  // Served Cell Information
//  /* Tracking area code */
//  uint16_t tac[M2AP_MAX_NB_CELLS];
//
//  /* Mobile Country Codes
//   * Mobile Network Codes
//   */
//  uint16_t mcc[M2AP_MAX_NB_CELLS];//[6];
//  uint16_t mnc[M2AP_MAX_NB_CELLS];//[6];
//  uint8_t  mnc_digit_length[M2AP_MAX_NB_CELLS];//[6];
//
//  // NR Global Cell Id
//  uint64_t nr_cellid[M2AP_MAX_NB_CELLS];
//  // NR Physical Cell Ids
//  uint16_t nr_pci[M2AP_MAX_NB_CELLS];
//  // Number of slide support items (max 16, could be increased to as much as 1024)
//  uint16_t num_ssi[M2AP_MAX_NB_CELLS];//[6];
//  uint8_t sst[M2AP_MAX_NB_CELLS];//[16][6];
//  uint8_t sd[M2AP_MAX_NB_CELLS];//[16][6];
//  // fdd_flag = 1 means FDD, 0 means TDD
//  int  fdd_flag;
//
//  /* eCGI->eCGI.pLMN_Identity */
//  uint16_t plmn_identity[M2AP_MAX_NB_CELLS];
//  /* eCGI->eCGI.eUTRANcellIdentifier */
//  uint16_t eutran_cell_identifier[M2AP_MAX_NB_CELLS];
// 
//  /* M2AP_MBSFN_SynchronisationArea_ID_t */
//  long mbsfn_synchronization_area[M2AP_MAX_NB_CELLS];
//
//  uint16_t service_area_id[M2AP_MAX_NB_CELLS][4];
//
// union {
//    struct {
//      uint32_t ul_nr_arfcn;
//      uint8_t ul_scs;
//      uint8_t ul_nrb;
//
//      uint32_t dl_nr_arfcn;
//      uint8_t dl_scs;
//      uint8_t dl_nrb;
//
//      uint32_t sul_active;
//      uint32_t sul_nr_arfcn;
//      uint8_t sul_scs;
//      uint8_t sul_nrb;
//
//      uint8_t ul_num_frequency_bands;
//      uint16_t ul_nr_band[32];
//      uint8_t ul_num_sul_frequency_bands;
//      uint16_t ul_nr_sul_band[32];
//
//      uint8_t dl_num_frequency_bands;
//      uint16_t dl_nr_band[32];
//      uint8_t dl_num_sul_frequency_bands;
//      uint16_t dl_nr_sul_band[32];
//    } fdd;
//    struct {
//
//      uint32_t nr_arfcn;
//      uint8_t scs;
//      uint8_t nrb;
//
//      uint32_t sul_active;
//      uint32_t sul_nr_arfcn;
//      uint8_t sul_scs;
//      uint8_t sul_nrb;
//
//      uint8_t num_frequency_bands;
//      uint16_t nr_band[32];
//      uint8_t num_sul_frequency_bands;
//      uint16_t nr_sul_band[32];
//
//    } tdd;
//  } nr_mode_info[M2AP_MAX_NB_CELLS];
//
//  char *measurement_timing_information[M2AP_MAX_NB_CELLS];
//  uint8_t ranac[M2AP_MAX_NB_CELLS];
//
//  // System Information
//  uint8_t *mib[M2AP_MAX_NB_CELLS];
//  int     mib_length[M2AP_MAX_NB_CELLS];
//  uint8_t *sib1[M2AP_MAX_NB_CELLS];
//  int     sib1_length[M2AP_MAX_NB_CELLS];


} m2ap_setup_req_t;


typedef struct m2ap_setup_resp_s {
 


  struct {
    uint8_t mbsfn_area;
    uint8_t pdcch_length;
    uint8_t repetition_period;
    uint8_t offset;
    uint8_t modification_period;
    uint8_t subframe_allocation_info;
    uint8_t mcs;
  } mcch_config_per_mbsfn[8];

  /* Connexion id used between SCTP/M2AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  
  uint64_t MCE_id;
  char     *MCE_name;

  uint8_t num_mcch_config_per_mbsfn;


//  /// string holding gNB_CU_name
//  char     *gNB_CU_name;
//  /// number of DU cells to activate
//  uint16_t num_cells_to_activate; //0< num_cells_to_activate <= 512;
//  /// mcc of DU cells
//  //uint16_t mcc[M2AP_MAX_NB_CELLS];
  uint16_t mcc;//[M2AP_MAX_NB_CELLS];
  /// mnc of DU cells
  //uint16_t mnc[M2AP_MAX_NB_CELLS];
  uint16_t mnc;//[M2AP_MAX_NB_CELLS];
  /// mnc digit length of DU cells
  //uint8_t mnc_digit_length[M2AP_MAX_NB_CELLS];
  uint8_t mnc_digit_length;//[M2AP_MAX_NB_CELLS];
  // NR Global Cell Id
//  uint64_t nr_cellid[M2AP_MAX_NB_CELLS];
//  /// NRPCI
//  uint16_t nrpci[M2AP_MAX_NB_CELLS];
//  /// num SI messages per DU cell
//  uint8_t num_SI[M2AP_MAX_NB_CELLS];
//  /// SI message containers (up to 21 messages per cell)
//  uint8_t *SI_container[M2AP_MAX_NB_CELLS][21];
//  int      SI_container_length[M2AP_MAX_NB_CELLS][21];

} m2ap_setup_resp_t;

typedef struct m2ap_setup_failure_s {
  uint16_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics;
} m2ap_setup_failure_t;



// eNB application layer -> M2AP messages

/* M2AP UE CONTEXT RELEASE */
//typedef struct m2ap_ue_context_release_s {
//  /* used for M2AP->RRC in source and RRC->M2AP in target */
//  int rnti;
//
//  int source_assoc_id;
//} m2ap_ue_context_release_t;

//typedef enum {
//  M2AP_T_RELOC_PREP_TIMEOUT,
//  M2AP_TX2_RELOC_OVERALL_TIMEOUT
//} m2ap_handover_cancel_cause_t;

//typedef struct m2ap_handover_cancel_s {
//  int rnti;
//  m2ap_handover_cancel_cause_t cause;
//} m2ap_handover_cancel_t;
typedef struct m2ap_register_mce_req_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t MCE_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char *MCE_name;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;

  /*
   * CC Params
   */
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int16_t                 N_RB_DL[MAX_NUM_CCs];
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_DL[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_UL[MAX_NUM_CCs];
  int                     num_cc;

  /* To be considered for TDD */
  //uint16_t tdd_EARFCN;
  //uint16_t tdd_Transmission_Bandwidth;

  /* The local eNB IP address to bind */
  net_ip_address_t mce_m2_ip_address;

  /* Nb of MME to connect to */
  uint8_t          nb_m2;

  /* List of target eNB to connect to for M2*/
  net_ip_address_t target_mce_m2_ip_address[M2AP_MAX_NB_ENB_IP_ADDRESS];

  /* Number of SCTP streams used for associations */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  /* eNB port for M2C*/
  uint32_t mce_port_for_M2C;

  /* timers (unit: millisecond) */
  int t_reloc_prep;
  int tm2_reloc_overall;
} m2ap_register_mce_req_t;


typedef struct m2ap_register_enb_req_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t eNB_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char *eNB_name;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;

  struct{
	  uint16_t mbsfn_sync_area;
  	  uint16_t mbms_service_area_list[8];
	  uint16_t num_mbms_service_area_list;
  }mbms_configuration_data_list[8];
  uint16_t num_mbms_configuration_data_list;



  /*
   * CC Params
   */
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int16_t                 N_RB_DL[MAX_NUM_CCs];
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_DL[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_UL[MAX_NUM_CCs];
  int                     num_cc;

  /* To be considered for TDD */
  //uint16_t tdd_EARFCN;
  //uint16_t tdd_Transmission_Bandwidth;

  /* The local eNB IP address to bind */
  net_ip_address_t enb_m2_ip_address;

  /* Nb of MME to connect to */
  uint8_t          nb_m2;

  /* List of target eNB to connect to for M2*/
  net_ip_address_t target_mce_m2_ip_address[M2AP_MAX_NB_ENB_IP_ADDRESS];

  /* Number of SCTP streams used for associations */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  /* eNB port for M2C*/
  uint32_t enb_port_for_M2C;

  /* timers (unit: millisecond) */
  int t_reloc_prep;
  int tm2_reloc_overall;
} m2ap_register_enb_req_t;

typedef struct m2ap_subframe_process_s {
  /* nothing, we simply use the module ID in the header */
} m2ap_subframe_process_t;

//-------------------------------------------------------------------------------------------//
// M2AP -> eNB application layer messages
typedef struct m2ap_register_enb_cnf_s {
  /* Nb of connected eNBs*/
  uint8_t          nb_m2;
} m2ap_register_enb_cnf_t;

typedef struct m2ap_deregistered_enb_ind_s {
  /* Nb of connected eNBs */
  uint8_t          nb_m2;
} m2ap_deregistered_enb_ind_t;


typedef struct m2ap_mbms_scheduling_information_s {
	uint16_t mcch_update_time;
	struct{
		uint8_t common_sf_allocation_period;
		uint8_t mbms_area_id;
		struct{
			uint8_t allocated_sf_end;
                        uint8_t data_mcs;
                        uint8_t mch_scheduling_period;
			struct{
				//struct{
				uint32_t service_id;
				uint16_t lcid;
				uint8_t mcc;
				uint8_t mnc;
				uint8_t mnc_length;
				//}mbms_session_per_pmch[8];
				//int num_mbms_session_per_pmch;
			}mbms_session_list[8];
			int num_mbms_session_list;
		}pmch_config_list[8];
		int num_pmch_config_list;

		struct{
			uint8_t radioframe_allocation_period;
			uint8_t radioframe_allocation_offset;
			uint8_t is_four_sf;
			uint32_t subframe_allocation;
		}mbms_sf_config_list[8];
		int num_mbms_sf_config_list;
		
	}mbms_area_config_list[8];
	uint8_t num_mbms_area_config_list;





  	uint16_t mcc[M2AP_MAX_NB_CELLS];//[6];
  	uint16_t mnc[M2AP_MAX_NB_CELLS];//[6];
  	uint8_t  mnc_digit_length[M2AP_MAX_NB_CELLS];//[6];
	uint8_t  TMGI[5]; // {4,3,2,1,0};
	uint8_t is_one_frame;
 	uint8_t buf1; //i.e 0x38<<2
 	uint8_t buf2; //i.e 0x38<<2
 	uint8_t buf3; //i.e 0x38<<2
	uint16_t common_subframe_allocation_period;
	
} m2ap_mbms_scheduling_information_t;


typedef struct m2ap_mce_sctp_req_s {
  	/* The local MCE IP address to bind */
	net_ip_address_t mce_m2_ip_address;

  	/* eNB port for M2C*/
  	uint32_t mce_port_for_M2C;
}m2ap_mce_sctp_req_t;

typedef struct m2ap_mbms_scheduling_information_resp_s {
} m2ap_mbms_scheduling_information_resp_t;
typedef struct m2ap_session_start_req_s {
} m2ap_session_start_req_t;
typedef struct m2ap_session_start_resp_s {
} m2ap_session_start_resp_t;
typedef struct m2ap_session_start_failure_s {
} m2ap_session_start_failure_t;
typedef struct m2ap_session_stop_req_s {
} m2ap_session_stop_req_t;
typedef struct m2ap_session_stop_resp_s {
} m2ap_session_stop_resp_t;
typedef struct m2ap_reset_s {
} m2ap_reset_t;
typedef struct m2ap_enb_configuration_update_s {
} m2ap_enb_configuration_update_t;
typedef struct m2ap_enb_configuration_update_ack_s {
} m2ap_enb_configuration_update_ack_t;
typedef struct m2ap_enb_configuration_update_failure_s {
} m2ap_enb_configuration_update_failure_t;
typedef struct m2ap_mce_configuration_update_s {
} m2ap_mce_configuration_update_t;
typedef struct m2ap_mce_configuration_update_ack_s {
} m2ap_mce_configuration_update_ack_t;
typedef struct m2ap_mce_configuration_update_failure_s {
} m2ap_mce_configuration_update_failure_t;
typedef struct m2ap_error_indication_s {
}m2ap_error_indication_t;
typedef struct m2ap_mbms_session_update_req_s {
} m2ap_mbms_session_update_req_t;
typedef struct m2ap_mbms_session_update_resp_s {
} m2ap_mbms_session_update_resp_t;
typedef struct m2ap_mbms_session_update_failure_s {
} m2ap_mbms_session_update_failure_t;
typedef struct m2ap_mbms_service_counting_report_s {
} m2ap_mbms_service_counting_report_t;
typedef struct m2ap_mbms_overload_notification_s {
} m2ap_mbms_overload_notification_t;
typedef struct m2ap_mbms_service_counting_req_s {
} m2ap_mbms_service_counting_req_t;
typedef struct m2ap_mbms_service_counting_resp_s {
} m2ap_mbms_service_counting_resp_t;
typedef struct m2ap_mbms_service_counting_failure_s {
} m2ap_mbms_service_counting_failure_t;


//-------------------------------------------------------------------------------------------//
// M2AP <-> RRC
//typedef struct m2ap_gummei_s {
//  uint16_t mcc;
//  uint16_t mnc;
//  uint8_t  mnc_len;
//  uint8_t  mme_code;
//  uint16_t mme_group_id;
//} m2ap_gummei_t;
//
//typedef struct m2ap_lastvisitedcell_info_s {
//  uint16_t mcc;
//  uint16_t mnc;
//  uint8_t  mnc_len;
//  LTE_PhysCellId_t target_physCellId;
//  cell_type_t cell_type;
//  uint64_t time_UE_StayedInCell;
//}m2ap_lastvisitedcell_info_t;
//
//typedef struct m2ap_handover_req_s {
//  /* used for RRC->M2AP in source eNB */
//  int rnti;
//
//  /* used for M2AP->RRC in target eNB */
//  int x2_id;
//
//  LTE_PhysCellId_t target_physCellId;
//
//  m2ap_gummei_t ue_gummei;
//
//  /*UE-ContextInformation */
//
//  /* MME UE id  */
//  uint32_t mme_ue_s1ap_id;
//
//  security_capabilities_t security_capabilities;
//
//  uint8_t      kenb[32]; // keNB or keNB*
//
//  /*next_hop_chaining_coun */
//  long int     kenb_ncc;
//
//  /* UE aggregate maximum bitrate */
//  ambr_t ue_ambr;
//
//  uint8_t nb_e_rabs_tobesetup;
//
// /* list of e_rab setup-ed by RRC layers */
//  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];
//
//  /* list of e_rab to be setup by RRC layers */
//  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];
//
//  m2ap_lastvisitedcell_info_t lastvisitedcell_info;
//
//  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
//  int rrc_buffer_size;
//
//  int target_assoc_id;
//} m2ap_handover_req_t;
//
//typedef struct m2ap_handover_req_ack_s {
//  /* used for RRC->M2AP in target and M2AP->RRC in source */
//  int rnti;
//
//  /* used for RRC->M2AP in target */
//  int x2_id_target;
//
//  int source_assoc_id;
//
//  uint8_t nb_e_rabs_tobesetup;
//
// /* list of e_rab setup-ed by RRC layers */
//  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];
//
//  /* list of e_rab to be setup by RRC layers */
//  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];
//
//  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
//  int rrc_buffer_size;
//
//  uint32_t mme_ue_s1ap_id;
//} m2ap_handover_req_ack_t;
//
#endif /* M2AP_MESSAGES_TYPES_H_ */
