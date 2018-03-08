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

/*
  nbiot_config.c
  -------------------
  AUTHOR  : Francois Taburet
  COMPANY : NOKIA
  EMAIL   : francois.taburet@nokia-bell-labs.com
*/

#include <string.h>
#include <inttypes.h>

#include "log.h"
#include "log_extern.h"
#include "assertions.h"
#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
# endif
#endif
#include "SystemInformationBlockType2.h"

#include "PHY/extern.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "RRC_paramsvalues.h"
#include "NB_IoT_paramdef.h"
#include "L1_paramdef.h"
#include "MACRLC_paramdef.h"
#include "LAYER2/MAC/proto_NB_IoT.h"


void RCconfig_NbIoTL1(void) {
  paramdef_t NbIoT_L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t NbIoT_L1_ParamList = {NBIOT_L1LIST_CONFIG_STRING,NULL,0};

/* No component carrier for NbIoT, ignore number of CC */
//  NbIoT_L1_Params[L1_CC_IDX ].paramflags = PARAMFLAG_DONOTREAD;

  config_getlist( &NbIoT_L1_ParamList,NbIoT_L1_Params,sizeof(NbIoT_L1_Params)/sizeof(paramdef_t), NULL);    
  if (NbIoT_L1_ParamList.numelt > 0) {
    if (RC.L1_NB_IoT == NULL) {
      RC.L1_NB_IoT                         = (PHY_VARS_eNB_NB_IoT **)malloc(RC.nb_nb_iot_L1_inst*sizeof(PHY_VARS_eNB_NB_IoT *));
      LOG_I(PHY,"RC.L1_NB_IoT = %p\n",RC.L1_NB_IoT);
      memset(RC.L1_NB_IoT,0,RC.nb_nb_iot_L1_inst*sizeof(PHY_VARS_eNB_NB_IoT *));
    }


  for(int j = 0; j <NbIoT_L1_ParamList.numelt ; j++) {
      if (RC.L1_NB_IoT[j] == NULL) {
	RC.L1_NB_IoT[j]                       = (PHY_VARS_eNB_NB_IoT *)malloc(sizeof(PHY_VARS_eNB_NB_IoT));
	LOG_I(PHY,"RC.L1_NB_IoT[%d] = %p\n",j,RC.L1_NB_IoT[j]);
	memset(RC.L1_NB_IoT[j],0,sizeof(PHY_VARS_eNB_NB_IoT));
      }
      if (strcmp(*(NbIoT_L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {

      }
      else if (strcmp(*(NbIoT_L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
	RC.L1_NB_IoT[j]->eth_params_n.local_if_name	       = strdup(*(NbIoT_L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
	RC.L1_NB_IoT[j]->eth_params_n.my_addr		      = strdup(*(NbIoT_L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
	RC.L1_NB_IoT[j]->eth_params_n.remote_addr	      = strdup(*(NbIoT_L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
	RC.L1_NB_IoT[j]->eth_params_n.my_portc  	      = *(NbIoT_L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
	RC.L1_NB_IoT[j]->eth_params_n.remote_portc	      = *(NbIoT_L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
	RC.L1_NB_IoT[j]->eth_params_n.my_portd  	      = *(NbIoT_L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
	RC.L1_NB_IoT[j]->eth_params_n.remote_portd	      = *(NbIoT_L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
	RC.L1_NB_IoT[j]->eth_params_n.transp_preference       = ETH_UDP_MODE;
      }
      
      else { // other midhaul
      }	
    }// j=0..num_inst
    printf("Initializing northbound interface for NB-IoT L1\n");
    l1_north_init_NB_IoT();
  } else {
    LOG_I(PHY,"No " NBIOT_L1LIST_CONFIG_STRING " configuration found");    
  }
}

void RCconfig_NbIoTmacrlc(void) {
 


  paramdef_t NbIoT_MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t NbIoT_MacRLC_ParamList = {NBIOT_MACRLCLIST_CONFIG_STRING,NULL,0};


/* No component carrier for NbIoT, ignore number of CC */
//  NbIoT_MacRLC_Params[MACRLC_CC_IDX ].paramflags = PARAMFLAG_DONOTREAD;

  config_getlist( &NbIoT_MacRLC_ParamList,NbIoT_MacRLC_Params,sizeof(NbIoT_MacRLC_Params)/sizeof(paramdef_t), NULL);    
  

  if ( NbIoT_MacRLC_ParamList.numelt > 0) {
    mac_top_init_eNB_NB_IoT();
    for (int j=0;j<RC.nb_nb_iot_macrlc_inst;j++) {

      if (strcmp(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
	// check number of instances is same as RRC/PDCP
	
      } else if (strcmp(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "cudu") == 0) {
	RC.nb_iot_mac[j]->eth_params_n.local_if_name            = strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_n.my_addr                  = strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_n.remote_addr              = strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_n.my_portc                 = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_n.remote_portc             = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_n.my_portd                 = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_n.remote_portd             = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
	RC.nb_iot_mac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
      } else { // other midhaul
	AssertFatal(1==0,"MACRLC %d: %s unknown northbound midhaul\n",j, *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr));
      }	

      if (strcmp(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_L1") == 0) {

	
      } else if (strcmp(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "nfapi") == 0) {
	RC.nb_iot_mac[j]->eth_params_s.local_if_name		= strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_IF_NAME_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_s.my_addr  		= strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_ADDRESS_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_s.remote_addr		= strdup(*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_ADDRESS_IDX].strptr));
	RC.nb_iot_mac[j]->eth_params_s.my_portc 		= *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTC_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_s.remote_portc             = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTC_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_s.my_portd                 = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTD_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_s.remote_portd             = *(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
	RC.nb_iot_mac[j]->eth_params_s.transp_preference        = ETH_UDP_MODE;
      } else { // other midhaul
	AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(NbIoT_MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      }	
    }// j=0..num_inst */
  } else {// MacRLC_ParamList.numelt > 0
	  AssertFatal (0,
		       "No " NBIOT_MACRLCLIST_CONFIG_STRING " configuration found");     
  }
}



	       
int RCconfig_NbIoTRRC(MessageDef *msg_p, int nbiotrrc_id,eNB_RRC_INST_NB_IoT *nbiotrrc) {


  char instprefix[MAX_OPTNAME_SIZE*3 + 32];
  
 
  checkedparam_t NBIoTCheckParams[] = NBIOT_RRCPARAMS_CHECK_DESC;
  paramdef_t     NBIoTParams[]      = NBIOTRRCPARAMS_DESC;

  paramdef_t     NBIoTPrachParams[]      = NBIOTRRC_NPRACH_PARAMS_DESC;
  checkedparam_t NBIoTPrachCheckParams[] = NBIOT_RRCLIST_NPRACHPARAMSCHECK_DESC;

  paramdef_t     NBIoTRRCRefParams[]      = NBIOTRRCPARAMS_RRCREF_DESC;

  paramdef_t     NBIoTLteCCParams[] = NBIOT_LTECCPARAMS_DESC;
  checkedparam_t NBIoTLteCCCheckParams[] = NBIOT_LTECCPARAMS_CHECK_DESC;
/* map parameter checking array instances to parameter definition array instances */
  for (int i=0; (i<sizeof(NBIoTParams)/sizeof(paramdef_t)) && (i<sizeof(NBIoTCheckParams)/sizeof(checkedparam_t)); i++ ) {
     NBIoTParams[i].chkPptr = &(NBIoTCheckParams[i]); 
  } 
  for (int i=0; (i<sizeof(NBIoTPrachParams)/sizeof(paramdef_t)) && (i<sizeof(NBIoTPrachCheckParams)/sizeof(checkedparam_t)); i++ ) {
     NBIoTPrachParams[i].chkPptr = &(NBIoTPrachCheckParams[i]);
  }

/* brut force itti message fields assignment, to be redesigned with itti replacement */
  NBIoTParams[NBIOT_RACH_RARESPONSEWINDOWSIZE_NB_IDX].uptr               = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).rach_raResponseWindowSize_NB);
  NBIoTParams[NBIOT_RACH_MACCONTENTIONRESOLUTIONTIMER_NB_IDX].uptr       = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).rach_macContentionResolutionTimer_NB);
  NBIoTParams[NBIOT_RACH_POWERRAMPINGSTEP_NB_IDX].uptr                   = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).rach_powerRampingStep_NB);		     		   
  NBIoTParams[NBIOT_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_NB_IDX].uptr = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).rach_preambleInitialReceivedTargetPower_NB);
  NBIoTParams[NBIOT_RACH_PREAMBLETRANSMAX_CE_NB_IDX].uptr                = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax_CE_NB);				    
  NBIoTParams[NBIOT_BCCH_MODIFICATIONPERIODCOEFF_NB_IDX].uptr            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).bcch_modificationPeriodCoeff_NB);	 
  NBIoTParams[NBIOT_PCCH_DEFAULTPAGINGCYCLE_NB_IDX].uptr                 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).pcch_defaultPagingCycle_NB);	 
  NBIoTParams[NBIOT_NPRACH_CP_LENGTH_IDX].uptr                           = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_CP_Length);  		 
  NBIoTParams[NBIOT_NPRACH_RSRP_RANGE_IDX].uptr                          = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_rsrp_range); 		       

  
  
  NBIoTParams[NBIOT_MAXNUMPREAMBLEATTEMPTCE_NB_IDX].uptr                 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_NB);

  NBIoTParams[NBIOT_NPDSCH_NRS_POWER_IDX].uptr  			 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npdsch_nrs_Power);       
  NBIoTParams[NBIOT_NPUSCH_ACK_NACK_NUMREPETITIONS_NB_IDX].uptr 	 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p). npusch_ack_nack_numRepetitions_NB);
  NBIoTParams[NBIOT_NPUSCH_SRS_SUBFRAMECONFIG_NB_IDX].uptr		 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p). npusch_srs_SubframeConfig_NB);
  NBIoTParams[NBIOT_NPUSCH_THREETONE_CYCLICSHIFT_R13_IDX].uptr 	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_threeTone_CyclicShift_r13);
  NBIoTParams[NBIOT_NPUSCH_SIXTONE_CYCLICSHIFT_R13_IDX].uptr             = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_sixTone_CyclicShift_r13);
  
  NBIoTParams[NBIOT_NPUSCH_GROUPASSIGNMENTNPUSCH_R13_IDX].uptr           = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_groupAssignmentNPUSCH_r13);
  NBIoTParams[NBIOT_DL_GAPTHRESHOLD_NB_IDX].uptr                         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).dl_GapThreshold_NB);	 
  NBIoTParams[NBIOT_DL_GAPPERIODICITY_NB_IDX].uptr                       = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).dl_GapPeriodicity_NB);	 
  
  NBIoTParams[NBIOT_NPUSCH_P0_NOMINALNPUSCH_IDX].uptr                    = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_p0_NominalNPUSCH);	  
  		  
  NBIoTParams[NBIOT_DELTAPREAMBLEMSG3_IDX].uptr                          = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).deltaPreambleMsg3); 	  
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_T300_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300_NB);						 
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_T301_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301_NB);
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_T310_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310_NB);						
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_T311_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311_NB);						
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_N310_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310_NB);						
  NBIoTParams[NBIOT_UE_TIMERSANDCONSTANTS_N311_NB_IDX].uptr              = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311_NB);

  sprintf(instprefix, NBIOT_RRCLIST_CONFIG_STRING ".[%i]",nbiotrrc_id);
  config_get( NBIoTParams,sizeof(NBIoTParams)/sizeof(paramdef_t),instprefix); 

  NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_SubcarrierMSG3_RangeStart    = config_get_processedint( &(NBIoTParams[NBIOT_NPRACH_SUBCARRIERMSG3_RANGESTART_IDX]) );
  NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_groupHoppingEnabled          = config_get_processedint( &(NBIoTParams[NBIOT_NPUSCH_GROUPHOPPINGENABLED_IDX]      ) ); 
  NBIOTRRC_CONFIGURATION_REQ (msg_p).dl_GapDurationCoeff_NB              = config_get_processedint( &(NBIoTParams[NBIOT_DL_GAPDURATIONCOEFF_NB_IDX]          ) ); 	 
  NBIOTRRC_CONFIGURATION_REQ (msg_p).npusch_alpha                        = config_get_processedint( &(NBIoTParams[NBIOT_NPUSCH_ALPHA_IDX]                    ) );
  for (int i=0; i<MAX_NUM_NBIOT_CELEVELS; i++) {
       char *tmpptr=NULL;
       NBIoTPrachParams[NBIOT_NPRACH_PERIODICITY_IDX ].uptr	            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_Periodicity[i]);		
       NBIoTPrachParams[NBIOT_NPRACH_STARTTIME_IDX].uptr  	            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_StartTime[i]);			
       NBIoTPrachParams[NBIOT_NPRACH_SUBCARRIEROFFSET_IDX].uptr	            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_SubcarrierOffset[i]);		
       NBIoTPrachParams[NBIOT_NPRACH_NUMSUBCARRIERS_IDX].uptr	            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).nprach_NumSubcarriers[i]);		
       NBIoTPrachParams[NBIOT_NUMREPETITIONSPERPREAMBLEATTEMPT_NB_IDX].uptr = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).numRepetitionsPerPreambleAttempt_NB[i]);
       NBIoTParams[NBIOT_NPDCCH_NUMREPETITIONS_RA_IDX].uptr                 = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npdcch_NumRepetitions_RA[i]);
       NBIoTParams[NBIOT_NPDCCH_STARTSF_CSS_RA_IDX].uptr	            = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).npdcch_StartSF_CSS_RA[i]);
       NBIoTParams[NBIOT_NPDCCH_OFFSET_RA_IDX].strptr			    = &tmpptr;
       sprintf(instprefix, "%s.[%i].%s.[%i]",NBIOT_RRCLIST_CONFIG_STRING, nbiotrrc_id,NBIOT_RRCLIST_NPRACHPARAMS_CONFIG_STRING,i);
       config_get( NBIoTPrachParams,sizeof(NBIoTPrachParams)/sizeof(paramdef_t),instprefix); 
       NBIOTRRC_CONFIGURATION_REQ (msg_p).npdcch_Offset_RA[i] = config_get_processedint( &(NBIoTPrachParams[NBIOT_NPDCCH_OFFSET_RA_IDX]) ); 
  }
/* get the LTE RRC and CC this NB-IoT RRC instance is attached to */
  sprintf(instprefix, NBIOT_RRCLIST_CONFIG_STRING ".[%i]." NBIOT_LTERRCREF_CONFIG_STRING, nbiotrrc_id );
  config_get( NBIoTRRCRefParams,sizeof(NBIoTRRCRefParams)/sizeof(paramdef_t),instprefix); 

/* read SIB1 parameters in the LTE RRC and CC sections */
  sprintf(instprefix, ENB_CONFIG_STRING_ENB_LIST ".[%i]."  ENB_CONFIG_STRING_COMPONENT_CARRIERS ".[%i]",
          *(NBIoTRRCRefParams[NBIOT_RRCINST_IDX].uptr), *(NBIoTRRCRefParams[NBIOT_CCINST_IDX].uptr)); 

  NBIoTLteCCParams[LTECCPARAMS_TDD_CONFIG_IDX  ].uptr	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).tdd_config);
  NBIoTLteCCParams[LTECCPARAMS_TDD_CONFIG_S_IDX].uptr	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).tdd_config_s);
  NBIoTLteCCParams[LTECCPARAMS_EUTRA_BAND_IDX ].uptr	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).eutra_band);             
  NBIoTLteCCParams[LTECCPARAMS_DOWNLINK_FREQUENCY_IDX].uptr      = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).downlink_frequency);      
  NBIoTLteCCParams[LTECCPARAMS_UPLINK_FREQUENCY_OFFSET_IDX].uptr = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset);
  NBIoTLteCCParams[LTECCPARAMS_NID_CELL_IDX].uptr	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).Nid_cell);                 
  NBIoTLteCCParams[LTECCPARAMS_N_RB_DL_IDX].uptr	         = (uint32_t *)&(NBIOTRRC_CONFIGURATION_REQ (msg_p).N_RB_DL);                   

  for (int i=0; (i<sizeof(NBIoTLteCCParams)/sizeof(paramdef_t)) && (i<sizeof(NBIoTLteCCCheckParams)/sizeof(checkedparam_t)); i++ ) {
     NBIoTLteCCParams[i].chkPptr = &(NBIoTLteCCCheckParams[i]);
  }
  config_get( NBIoTLteCCParams,sizeof(NBIoTLteCCParams)/sizeof(paramdef_t),instprefix); 
  NBIOTRRC_CONFIGURATION_REQ (msg_p).frame_type = config_get_processedint( &(NBIoTLteCCParams[LTECCPARAMS_FRAME_TYPE_IDX]) ); 
  NBIOTRRC_CONFIGURATION_REQ (msg_p).prefix_type = config_get_processedint( &(NBIoTLteCCParams[LTECCPARAMS_PREFIX_TYPE_IDX]) );
  NBIOTRRC_CONFIGURATION_REQ (msg_p).prefix_type = config_get_processedint( &(NBIoTLteCCParams[LTECCPARAMS_PREFIX_TYPE_UL_IDX]) );
return 0;
}

void RCConfig_NbIoT(RAN_CONTEXT_t *RC) {

  paramlist_def_t NbIoT_MACRLCParamList = {NBIOT_MACRLCLIST_CONFIG_STRING,NULL,0};
  paramlist_def_t NbIoT_L1ParamList = {NBIOT_L1LIST_CONFIG_STRING,NULL,0};
  paramlist_def_t NbIoT_ParamList = {NBIOT_RRCLIST_CONFIG_STRING,NULL,0};
    
  
  config_getlist( &NbIoT_ParamList,NULL,0,NULL);
  RC->nb_nb_iot_rrc_inst = NbIoT_ParamList.numelt;


    
 
    config_getlist( &NbIoT_MACRLCParamList,NULL,0, NULL);
    RC->nb_nb_iot_macrlc_inst  = NbIoT_MACRLCParamList.numelt;
    // Get num L1 instances
    config_getlist( &NbIoT_L1ParamList,NULL,0, NULL);
    RC->nb_nb_iot_L1_inst = NbIoT_L1ParamList.numelt;

}
