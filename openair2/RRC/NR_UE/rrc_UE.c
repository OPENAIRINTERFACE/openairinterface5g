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

/* \file rrc_UE.c
 * \brief RRC procedures
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#define RRC_UE
#define RRC_UE_C

#include "NR_DL-DCCH-Message.h"     //asn_DEF_NR_DL_DCCH_Message
#include "NR_BCCH-BCH-Message.h"    //asn_DEF_NR_BCCH_BCH_Message
#include "NR_CellGroupConfig.h"     //asn_DEF_NR_CellGroupConfig
#include "NR_BWP-Downlink.h"        //asn_DEF_NR_BWP_Downlink
#include "NR_RRCReconfiguration.h"
#include "NR_MeasConfig.h"
#include "NR_UL-DCCH-Message.h"

#include "rrc_list.h"
#include "rrc_defs.h"
#include "rrc_proto.h"
#include "rrc_vars.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

#include "executables/softmodem-common.h"
#include "pdcp.h"
#include "UTIL/OSA/osa_defs.h"

mui_t nr_rrc_mui=0;

extern boolean_t nr_rrc_pdcp_config_asn1_req(
    const protocol_ctxt_t *const  ctxt_pP,
    NR_SRB_ToAddModList_t  *const srb2add_list,
    NR_DRB_ToAddModList_t  *const drb2add_list,
    NR_DRB_ToReleaseList_t *const drb2release_list,
    const uint8_t                   security_modeP,
    uint8_t                  *const kRRCenc,
    uint8_t                  *const kRRCint,
    uint8_t                  *const kUPenc
  #if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
    ,LTE_PMCH_InfoList_r9_t  *pmch_InfoList_r9
  #endif
    ,rb_id_t                 *const defaultDRB,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

extern rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
int8_t nr_rrc_ue_decode_secondary_cellgroup_config(
    const uint8_t *buffer,
    const uint32_t size ){
    
    NR_CellGroupConfig_t *cell_group_config = NULL;
    uint32_t i;

    asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                                                    &asn_DEF_NR_CellGroupConfig,
                                                    (void **)&cell_group_config,
                                                    (uint8_t *)buffer,
                                                    size ); 

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
            printf("NR_CellGroupConfig decode error\n");
            for (i=0; i<size; i++){
                printf("%02x ",buffer[i]);
            }
            printf("\n");
            // free the memory
            SEQUENCE_free( &asn_DEF_NR_CellGroupConfig, (void *)cell_group_config, 1 );
            return -1;
    }

    if(NR_UE_rrc_inst->cell_group_config == NULL){
        NR_UE_rrc_inst->cell_group_config = cell_group_config;
        nr_rrc_ue_process_scg_config(cell_group_config);
    }else{
        nr_rrc_ue_process_scg_config(cell_group_config);
        SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)cell_group_config, 0);
    }

    //nr_rrc_mac_config_req_ue( 0,0,0,NULL, cell_group_config->mac_CellGroupConfig, cell_group_config->physicalCellGroupConfig, cell_group_config->spCellConfig );

    return 0;
}

int8_t nr_rrc_ue_process_RadioBearerConfig(NR_RadioBearerConfig_t *RadioBearerConfig){


  xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void*)RadioBearerConfig);
  // Configure PDCP

  return 0;
}

// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (decoded)
// RRCReconfiguration
int8_t nr_rrc_ue_process_rrcReconfiguration(NR_RRCReconfiguration_t *rrcReconfiguration){

    switch(rrcReconfiguration->criticalExtensions.present){
        case NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration:
            if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig != NULL){
                if(NR_UE_rrc_inst->radio_bearer_config == NULL){
                    NR_UE_rrc_inst->radio_bearer_config = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig;                
                }else{
                    nr_rrc_ue_process_RadioBearerConfig(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->radioBearerConfig);
                }
            }

            if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup != NULL){
                NR_CellGroupConfig_t *cellGroupConfig = NULL;
                uper_decode(NULL,
                            &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
                            (void **)&cellGroupConfig,
                            (uint8_t *)rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->buf,
                            rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->secondaryCellGroup->size, 0, 0); 

		xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void*)cellGroupConfig);

                if(NR_UE_rrc_inst->cell_group_config == NULL){
                    //  first time receive the configuration, just use the memory allocated from uper_decoder. TODO this is not good implementation, need to maintain RRC_INST own structure every time.
                    NR_UE_rrc_inst->cell_group_config = cellGroupConfig;
                    nr_rrc_ue_process_scg_config(cellGroupConfig);
                }else{
                    //  after first time, update it and free the memory after.
                    SEQUENCE_free(&asn_DEF_NR_CellGroupConfig, (void *)NR_UE_rrc_inst->cell_group_config, 0);
                    NR_UE_rrc_inst->cell_group_config = cellGroupConfig;
                    nr_rrc_ue_process_scg_config(cellGroupConfig);
                }
                
            }

            if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig != NULL){
                if(NR_UE_rrc_inst->meas_config == NULL){
                    NR_UE_rrc_inst->meas_config = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig;
                }else{
                    //  if some element need to be updated
                    nr_rrc_ue_process_meas_config(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->measConfig);
                }
               
            }

            if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->lateNonCriticalExtension != NULL){
                //  unuse now
            }

            if(rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration->nonCriticalExtension != NULL){
                // unuse now
            }
            break;
        case NR_RRCReconfiguration__criticalExtensions_PR_NOTHING:
        case NR_RRCReconfiguration__criticalExtensions_PR_criticalExtensionsFuture:
        default:
            break;
    }
    //nr_rrc_mac_config_req_ue(); 

    return 0;
}



int8_t nr_rrc_ue_process_meas_config(NR_MeasConfig_t *meas_config){

    return 0;
}

int8_t nr_rrc_ue_process_scg_config(NR_CellGroupConfig_t *cell_group_config){
    int i;
    if(NR_UE_rrc_inst->cell_group_config==NULL){
      //  initial list
        if(cell_group_config->spCellConfig != NULL){
            if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
                if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
                    for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count; ++i){
                        RRC_LIST_MOD_ADD(NR_UE_rrc_inst->BWP_Downlink_list, cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[i], bwp_Id);
                    }
                }
            }
        } 
    }else{
        //  maintain list
        if(cell_group_config->spCellConfig != NULL){
            if(cell_group_config->spCellConfig->spCellConfigDedicated != NULL){
                //  process element of list to be add by RRC message
                if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList != NULL){
                    for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count; ++i){
                        RRC_LIST_MOD_ADD(NR_UE_rrc_inst->BWP_Downlink_list, cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[i], bwp_Id);
                    }
                }
                

                //  process element of list to be release by RRC message
                if(cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList != NULL){
                    for(i=0; i<cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList->list.count; ++i){
                        NR_BWP_Downlink_t *freeP = NULL;
                        RRC_LIST_MOD_REL(NR_UE_rrc_inst->BWP_Downlink_list, bwp_Id, *cell_group_config->spCellConfig->spCellConfigDedicated->downlinkBWP_ToReleaseList->list.array[i], freeP);
                        if(freeP != NULL){
                            SEQUENCE_free(&asn_DEF_NR_BWP_Downlink, (void *)freeP, 0);
                        }
                    }
                }
            }
        }
    } 

    return 0;
}




void process_nsa_message(NR_UE_RRC_INST_t *rrc, nsa_message_t nsa_message_type, void *message,int msg_len) {

  switch (nsa_message_type) {
    case nr_SecondaryCellGroupConfig_r15:
      {
	NR_RRCReconfiguration_t *RRCReconfiguration=NULL;
	asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
							&asn_DEF_NR_RRCReconfiguration,
							(void **)&RRCReconfiguration,
							(uint8_t *)message,
							msg_len); 
	
	if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
	  LOG_E(RRC,"NR_RRCReconfiguration decode error\n");
	  // free the memory
	  SEQUENCE_free( &asn_DEF_NR_RRCReconfiguration, RRCReconfiguration, 1 );
	  return;
	}      
	nr_rrc_ue_process_rrcReconfiguration(RRCReconfiguration);
      }
      break;
    case nr_RadioBearerConfigX_r15:
      {
	NR_RadioBearerConfig_t *RadioBearerConfig=NULL;
	asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
							&asn_DEF_NR_RadioBearerConfig,
							(void **)&RadioBearerConfig,
							(uint8_t *)message,
							msg_len); 
	
	if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
	  LOG_E(RRC,"NR_RadioBearerConfig decode error\n");
	  // free the memory
	  SEQUENCE_free( &asn_DEF_NR_RadioBearerConfig, RadioBearerConfig, 1 );
	  return;
	}      
	nr_rrc_ue_process_RadioBearerConfig(RadioBearerConfig);
      }
      break;
    default:
      AssertFatal(1==0,"Unknown message %d\n",nsa_message_type);
      break;
  }

}

NR_UE_RRC_INST_t* openair_rrc_top_init_ue_nr(char* rrc_config_path){

    if(NB_NR_UE_INST > 0){
        NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)malloc(NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));
        memset(NR_UE_rrc_inst, 0, NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));

        // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.

        //  init RRC lists
        RRC_LIST_INIT(NR_UE_rrc_inst->RLC_Bearer_Config_list, NR_maxLC_ID);
        RRC_LIST_INIT(NR_UE_rrc_inst->SchedulingRequest_list, NR_maxNrofSR_ConfigPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->TAG_list, NR_maxNrofTAGs);
        RRC_LIST_INIT(NR_UE_rrc_inst->TDD_UL_DL_SlotConfig_list, NR_maxNrofSlots);
        RRC_LIST_INIT(NR_UE_rrc_inst->BWP_Downlink_list, NR_maxNrofBWPs);
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[0], 3);   //  for init-dl-bwp
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[1], 3);   //  for dl-bwp id=0
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[2], 3);   //  for dl-bwp id=1
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[3], 3);   //  for dl-bwp id=2
        RRC_LIST_INIT(NR_UE_rrc_inst->ControlResourceSet_list[4], 3);   //  for dl-bwp id=3
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list[0], 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list[1], 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list[2], 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list[3], 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SearchSpace_list[4], 10);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list[0], NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list[1], NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list[2], NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list[3], NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->SlotFormatCombinationsPerCell_list[4], NR_maxNrofAggregatedCellsPerCellGroup);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list[0], NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list[1], NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list[2], NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list[3], NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->TCI_State_list[4], NR_maxNrofTCI_States);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list[0], NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list[1], NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list[2], NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list[3], NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->RateMatchPattern_list[4], NR_maxNrofRateMatchPatterns);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list[0], NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list[1], NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list[2], NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list[3], NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->ZP_CSI_RS_Resource_list[4], NR_maxNrofZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list[0], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list[1], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list[2], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list[3], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->Aperidic_ZP_CSI_RS_ResourceSet_list[4], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list[0], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list[1], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list[2], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list[3], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->SP_ZP_CSI_RS_ResourceSet_list[4], NR_maxNrofZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->NZP_CSI_RS_Resource_list, NR_maxNrofNZP_CSI_RS_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->NZP_CSI_RS_ResourceSet_list, NR_maxNrofNZP_CSI_RS_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_IM_Resource_list, NR_maxNrofCSI_IM_Resources);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_IM_ResourceSet_list, NR_maxNrofCSI_IM_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_SSB_ResourceSet_list, NR_maxNrofCSI_SSB_ResourceSets);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_ResourceConfig_list, NR_maxNrofCSI_ResourceConfigurations);
        RRC_LIST_INIT(NR_UE_rrc_inst->CSI_ReportConfig_list, NR_maxNrofCSI_ReportConfigurations);

	if (get_softmodem_params()->phy_test==1 || get_softmodem_params()->do_ra==1) {
	  // read in files for RRCReconfiguration and RBconfig
	  FILE *fd;
	  char filename[1024];
	  if (rrc_config_path)
	    sprintf(filename,"%s/reconfig.raw",rrc_config_path);
	  else
	    sprintf(filename,"reconfig.raw");
	  fd = fopen(filename,"r");
          char buffer[1024];
	  AssertFatal(fd,
	              "cannot read file %s: errno %d, %s\n",
	              filename,
	              errno,
	              strerror(errno));
	  int msg_len=fread(buffer,1,1024,fd);
	  fclose(fd);
	  process_nsa_message(NR_UE_rrc_inst, nr_SecondaryCellGroupConfig_r15, buffer,msg_len);
	  if (rrc_config_path)
	    sprintf(filename,"%s/rbconfig.raw",rrc_config_path);
	  else
	    sprintf(filename,"rbconfig.raw");
	  fd = fopen(filename,"r");
	  AssertFatal(fd,
	              "cannot read file %s: errno %d, %s\n",
	              filename,
	              errno,
	              strerror(errno));
	  msg_len=fread(buffer,1,1024,fd);
	  fclose(fd);
	  process_nsa_message(NR_UE_rrc_inst, nr_RadioBearerConfigX_r15, buffer,msg_len); 
	}
    }else{
        NR_UE_rrc_inst = NULL;
    }

    return NR_UE_rrc_inst;
}


int8_t nr_ue_process_rlc_bearer_list(NR_CellGroupConfig_t *cell_group_config){

    return 0;
}

int8_t nr_ue_process_secondary_cell_list(NR_CellGroupConfig_t *cell_group_config){

    return 0;
}

int8_t nr_ue_process_mac_cell_group_config(NR_MAC_CellGroupConfig_t *mac_cell_group_config){

    return 0;
}

int8_t nr_ue_process_physical_cell_group_config(NR_PhysicalCellGroupConfig_t *phy_cell_group_config){

    return 0;
}

int8_t nr_ue_process_spcell_config(NR_SpCellConfig_t *spcell_config){

    return 0;
}

/*brief decode BCCH-BCH (MIB) message*/
int8_t nr_rrc_ue_decode_NR_BCCH_BCH_Message(
    const module_id_t module_id,
    const uint8_t     gNB_index,
    uint8_t           *const bufferP,
    const uint8_t     buffer_len ){
    int i;
    NR_BCCH_BCH_Message_t *bcch_message = NULL;
    NR_MIB_t *mib = NR_UE_rrc_inst->mib;

    if(mib != NULL){
        SEQUENCE_free( &asn_DEF_NR_BCCH_BCH_Message, (void *)mib, 1 );
    }


    for(i=0; i<buffer_len; ++i){
        printf("[RRC] MIB PDU : %d\n", bufferP[i]);
    }

    asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                                                    &asn_DEF_NR_BCCH_BCH_Message,
                                                   (void **)&bcch_message,
                                                   (const void *)bufferP,
                                                   buffer_len );

    if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {
      printf("NR_BCCH_BCH decode error\n");
      for (i=0; i<buffer_len; i++){
	printf("%02x ",bufferP[i]);
      }
      printf("\n");
      // free the memory
      SEQUENCE_free( &asn_DEF_NR_BCCH_BCH_Message, (void *)bcch_message, 1 );
      return -1;
    }
    else {
      //  link to rrc instance
      mib = bcch_message->message.choice.mib;
      //memcpy( (void *)mib,
      //    (void *)&bcch_message->message.choice.mib,
      //    sizeof(NR_MIB_t) );
      
      nr_rrc_mac_config_req_ue( 0, 0, 0, mib, NULL);
    }
    
    return 0;
}


// from NR SRB3
int8_t nr_rrc_ue_decode_NR_DL_DCCH_Message(
    const module_id_t module_id,
    const uint8_t     gNB_index,
    const uint8_t    *bufferP,
    const uint32_t    buffer_len ){
    //  uper_decode by nr R15 rrc_connection_reconfiguration
    
    int32_t i;
    NR_DL_DCCH_Message_t *nr_dl_dcch_msg = NULL;

    asn_dec_rval_t dec_rval = uper_decode(  NULL,
                                            &asn_DEF_NR_DL_DCCH_Message,    
                                            (void**)&nr_dl_dcch_msg,
                                            (uint8_t *)bufferP,
                                            buffer_len, 0, 0);

    if ((dec_rval.code != RC_OK) || (dec_rval.consumed == 0)) {

        for (i=0; i<buffer_len; i++)
            printf("%02x ",bufferP[i]);

        printf("\n");
        // free the memory
        SEQUENCE_free( &asn_DEF_NR_DL_DCCH_Message, (void *)nr_dl_dcch_msg, 1 );
        return -1;
    }

    if(nr_dl_dcch_msg != NULL){
        switch(nr_dl_dcch_msg->message.present){            
            case NR_DL_DCCH_MessageType_PR_c1:

                switch(nr_dl_dcch_msg->message.choice.c1->present){
                    case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
                        nr_rrc_ue_process_rrcReconfiguration(nr_dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration);
                        break;

                    case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
                    case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
                    case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
                    case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
                    case NR_DL_DCCH_MessageType__c1_PR_securityModeCommand:
                    case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
                    case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
                    case NR_DL_DCCH_MessageType__c1_PR_counterCheck:
                    case NR_DL_DCCH_MessageType__c1_PR_mobilityFromNRCommand:
                    case NR_DL_DCCH_MessageType__c1_PR_dlDedicatedMessageSegment_r16:
                    case NR_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r16:
                    case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransferMRDC_r16:
                    case NR_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r16:
                    case NR_DL_DCCH_MessageType__c1_PR_spare3:
                    case NR_DL_DCCH_MessageType__c1_PR_spare2:
                    case NR_DL_DCCH_MessageType__c1_PR_spare1:
                    default:
                        //  not supported or unused
                        break;
                }   
                break;
            case NR_DL_DCCH_MessageType_PR_NOTHING:
            case NR_DL_DCCH_MessageType_PR_messageClassExtension:
            default:
                //  not supported or unused
                break;
        }
        
        //  release memory allocation
        SEQUENCE_free( &asn_DEF_NR_DL_DCCH_Message, (void *)nr_dl_dcch_msg, 1 );
    }else{
        //  log..
    }

    return 0;

}


//-----------------------------------------------------------------------------
void
nr_rrc_ue_process_securityModeCommand(
  const protocol_ctxt_t *const ctxt_pP,
  NR_SecurityModeCommand_t *const securityModeCommand,
  const uint8_t                gNB_index
)
//-----------------------------------------------------------------------------
{
  asn_enc_rval_t enc_rval;
  NR_UL_DCCH_Message_t ul_dcch_msg;
  uint8_t buffer[200];
  int i, securityMode;
  LOG_I(RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), Processing securityModeCommand (eNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, gNB_index);

  switch (securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm) {
    case NR_CipheringAlgorithm_nea0:
      LOG_I(RRC,"[UE %d] Security algorithm is set to nea0\n",
            ctxt_pP->module_id);
      securityMode= NR_CipheringAlgorithm_nea0;
      break;

    case NR_CipheringAlgorithm_nea1:
      LOG_I(RRC,"[UE %d] Security algorithm is set to nea1\n",ctxt_pP->module_id);
      securityMode= NR_CipheringAlgorithm_nea1;
      break;

    case NR_CipheringAlgorithm_nea2:
      LOG_I(RRC,"[UE %d] Security algorithm is set to nea2\n",
            ctxt_pP->module_id);
      securityMode = NR_CipheringAlgorithm_nea2;
      break;

    default:
      LOG_I(RRC,"[UE %d] Security algorithm is set to none\n",ctxt_pP->module_id);
      securityMode = NR_CipheringAlgorithm_spare1;
      break;
  }

  switch (*securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm) {
    case NR_IntegrityProtAlgorithm_nia1:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to nia1\n",ctxt_pP->module_id);
      securityMode |= 1 << 5;
      break;

    case NR_IntegrityProtAlgorithm_nia2:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to nia2\n",ctxt_pP->module_id);
      securityMode |= 1 << 6;
      break;

    default:
      LOG_I(RRC,"[UE %d] Integrity protection algorithm is set to none\n",ctxt_pP->module_id);
      securityMode |= 0x70 ;
      break;
  }

  LOG_D(RRC,"[UE %d] security mode is %x \n",ctxt_pP->module_id, securityMode);
  NR_UE_rrc_inst->cipheringAlgorithm =
    securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.cipheringAlgorithm;
  NR_UE_rrc_inst->integrityProtAlgorithm =
    *securityModeCommand->criticalExtensions.choice.securityModeCommand->securityConfigSMC.securityAlgorithmConfig.integrityProtAlgorithm;
memset((void *)&ul_dcch_msg,0,sizeof(NR_UL_DCCH_Message_t));
  //memset((void *)&SecurityModeCommand,0,sizeof(SecurityModeCommand_t));
  ul_dcch_msg.message.present           = NR_UL_DCCH_MessageType_PR_c1;
  ul_dcch_msg.message.choice.c1         = calloc(1, sizeof(*ul_dcch_msg.message.choice.c1));

  if (securityMode >= NO_SECURITY_MODE) {
    LOG_I(RRC, "rrc_ue_process_securityModeCommand, security mode complete case \n");
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeComplete;
  } else {
    LOG_I(RRC, "rrc_ue_process_securityModeCommand, security mode failure case \n");
    ul_dcch_msg.message.choice.c1->present = NR_UL_DCCH_MessageType__c1_PR_securityModeFailure;
  }

  uint8_t *kRRCenc = NULL;
  uint8_t *kUPenc = NULL;
  uint8_t *kRRCint = NULL;
  pdcp_t *pdcp_p = NULL;
  hash_key_t key = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t h_rc;
  key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti,
                            ctxt_pP->enb_flag, DCCH, SRB_FLAG_YES);
  h_rc = hashtable_get(pdcp_coll_p, key, (void **) &pdcp_p);

  if (h_rc == HASH_TABLE_OK) {
    LOG_D(RRC, "PDCP_COLL_KEY_VALUE() returns valid key = %ld\n", key);
    LOG_D(RRC, "driving kRRCenc, kRRCint and kUPenc from KgNB="
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x"
          "%02x%02x%02x%02x\n",
          NR_UE_rrc_inst->kgnb[0],  NR_UE_rrc_inst->kgnb[1],  NR_UE_rrc_inst->kgnb[2],  NR_UE_rrc_inst->kgnb[3],
          NR_UE_rrc_inst->kgnb[4],  NR_UE_rrc_inst->kgnb[5],  NR_UE_rrc_inst->kgnb[6],  NR_UE_rrc_inst->kgnb[7],
          NR_UE_rrc_inst->kgnb[8],  NR_UE_rrc_inst->kgnb[9],  NR_UE_rrc_inst->kgnb[10], NR_UE_rrc_inst->kgnb[11],
          NR_UE_rrc_inst->kgnb[12], NR_UE_rrc_inst->kgnb[13], NR_UE_rrc_inst->kgnb[14], NR_UE_rrc_inst->kgnb[15],
          NR_UE_rrc_inst->kgnb[16], NR_UE_rrc_inst->kgnb[17], NR_UE_rrc_inst->kgnb[18], NR_UE_rrc_inst->kgnb[19],
          NR_UE_rrc_inst->kgnb[20], NR_UE_rrc_inst->kgnb[21], NR_UE_rrc_inst->kgnb[22], NR_UE_rrc_inst->kgnb[23],
          NR_UE_rrc_inst->kgnb[24], NR_UE_rrc_inst->kgnb[25], NR_UE_rrc_inst->kgnb[26], NR_UE_rrc_inst->kgnb[27],
          NR_UE_rrc_inst->kgnb[28], NR_UE_rrc_inst->kgnb[29], NR_UE_rrc_inst->kgnb[30], NR_UE_rrc_inst->kgnb[31]);
    derive_key_rrc_enc(NR_UE_rrc_inst->cipheringAlgorithm,NR_UE_rrc_inst->kgnb, &kRRCenc);
    derive_key_rrc_int(NR_UE_rrc_inst->integrityProtAlgorithm,NR_UE_rrc_inst->kgnb, &kRRCint);
    derive_key_up_enc(NR_UE_rrc_inst->cipheringAlgorithm,NR_UE_rrc_inst->kgnb, &kUPenc);

    if (securityMode != 0xff) {
      pdcp_config_set_security(ctxt_pP, pdcp_p, 0, 0,
                               NR_UE_rrc_inst->cipheringAlgorithm
                               | (NR_UE_rrc_inst->integrityProtAlgorithm << 4),
                               kRRCenc, kRRCint, kUPenc);
    } else {
      LOG_I(RRC, "skipped pdcp_config_set_security() as securityMode == 0x%02x",
            securityMode);
    }
  } else {
    LOG_I(RRC, "Could not get PDCP instance where key=0x%ld\n", key);
  }

  if (securityModeCommand->criticalExtensions.present == NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand) {

    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->rrc_TransactionIdentifier = securityModeCommand->rrc_TransactionIdentifier;
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->criticalExtensions.present = NR_SecurityModeComplete__criticalExtensions_PR_securityModeComplete;
    ul_dcch_msg.message.choice.c1->choice.securityModeComplete->criticalExtensions.choice.securityModeComplete->nonCriticalExtension =NULL;
    LOG_I(RRC,"[UE %d] SFN/SF %d/%d: Receiving from SRB1 (DL-DCCH), encoding securityModeComplete (eNB %d), rrc_TransactionIdentifier: %ld\n",
          ctxt_pP->module_id,ctxt_pP->frame, ctxt_pP->subframe, gNB_index, securityModeCommand->rrc_TransactionIdentifier);
    enc_rval = uper_encode_to_buffer(&asn_DEF_NR_UL_DCCH_Message,
                                     NULL,
                                     (void *)&ul_dcch_msg,
                                     buffer,
                                     100);
    AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %jd)!\n",
                 enc_rval.failed_type->name, enc_rval.encoded);

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
      xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)&ul_dcch_msg);
    }

    LOG_D(RRC, "securityModeComplete Encoded %zd bits (%zd bytes)\n", enc_rval.encoded, (enc_rval.encoded+7)/8);

    for (i = 0; i < (enc_rval.encoded + 7) / 8; i++) {
      LOG_T(RRC, "%02x.", buffer[i]);
    }

    LOG_T(RRC, "\n");
    rrc_data_req (
      ctxt_pP,
      DCCH,
      nr_rrc_mui++,
      SDU_CONFIRM_NO,
      (enc_rval.encoded + 7) / 8,
      buffer,
      PDCP_TRANSMISSION_MODE_CONTROL);
  } else LOG_W(RRC,"securityModeCommand->criticalExtensions.present (%d) != NR_SecurityModeCommand__criticalExtensions_PR_securityModeCommand\n",
                 securityModeCommand->criticalExtensions.present);
}

//-----------------------------------------------------------------------------
void rrc_ue_generate_RRCSetupRequest( const protocol_ctxt_t *const ctxt_pP, const uint8_t gNB_index ) {
  uint8_t i=0,rv[6];

  if(NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.payload_size ==0) {
    // Get RRCConnectionRequest, fill random for now
    // Generate random byte stream for contention resolution
    for (i=0; i<6; i++) {
#ifdef SMBV
      // if SMBV is configured the contention resolution needs to be fix for the connection procedure to succeed
      rv[i]=i;
#else
      rv[i]=taus()&0xff;
#endif
      LOG_T(RRC,"%x.",rv[i]);
    }

    LOG_T(RRC,"\n");
    NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.payload_size =
      do_RRCSetupRequest(
        ctxt_pP->module_id,
        (uint8_t *)NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.Payload,
        rv);
    LOG_I(RRC,"[UE %d] : Frame %d, Logical Channel UL-CCCH (SRB0), Generating RRCSetupRequest (bytes %d, eNB %d)\n",
          ctxt_pP->module_id, ctxt_pP->frame, NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.payload_size, gNB_index);

    for (i=0; i<NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.payload_size; i++) {
      LOG_T(RRC,"%x.",NR_UE_rrc_inst[ctxt_pP->module_id].Srb0[gNB_index].Tx_buffer.Payload[i]);
    }

    LOG_T(RRC,"\n");
    /*UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.Payload[i] = taus()&0xff;
    UE_rrc_inst[ue_mod_idP].Srb0[Idx].Tx_buffer.payload_size =i; */
  }
}

//-----------------------------------------------------------------------------
int32_t
nr_rrc_ue_establish_srb1(
    module_id_t       ue_mod_idP,
    frame_t           frameP,
    uint8_t           gNB_index,
    NR_SRB_ToAddMod_t *SRB_config
)
//-----------------------------------------------------------------------------
{
    // add descriptor from RRC PDU
    NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].Active = 1;
    NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].Status = RADIO_CONFIG_OK;//RADIO CFG
    NR_UE_rrc_inst[ue_mod_idP].Srb1[gNB_index].Srb_info.Srb_id = 1;
    // copy default configuration for now
    //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Lchan_desc[0],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
    //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Lchan_desc[1],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
    LOG_I(NR_RRC, "[UE %d], CONFIG_SRB1 %d corresponding to gNB_index %d\n", ue_mod_idP, DCCH, gNB_index);
    //rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, lchan_id,UNDEF_SECURITY_MODE);
    //  rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,lchan_id,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
    //  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size=DEFAULT_MEAS_IND_SIZE+1;
    return(0);
}

//-----------------------------------------------------------------------------
int32_t
nr_rrc_ue_establish_srb2(
    module_id_t       ue_mod_idP,
    frame_t           frameP,
    uint8_t           gNB_index,
    NR_SRB_ToAddMod_t *SRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].Active = 1;
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].Status = RADIO_CONFIG_OK;//RADIO CFG
  NR_UE_rrc_inst[ue_mod_idP].Srb2[gNB_index].Srb_info.Srb_id = 2;
  // copy default configuration for now
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Srb_info.Lchan_desc[0],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  //  memcpy(&UE_rrc_inst[ue_mod_idP].Srb2[eNB_index].Srb_info.Lchan_desc[1],&DCCH_LCHAN_DESC,LCHAN_DESC_SIZE);
  LOG_I(NR_RRC, "[UE %d], CONFIG_SRB2 %d corresponding to gNB_index %d\n", ue_mod_idP, DCCH1, gNB_index);
  //rrc_pdcp_config_req (ue_mod_idP+NB_eNB_INST, frameP, 0, CONFIG_ACTION_ADD, lchan_id, UNDEF_SECURITY_MODE);
  //  rrc_rlc_config_req(ue_mod_idP+NB_eNB_INST,frameP,0,CONFIG_ACTION_ADD,lchan_id,SIGNALLING_RADIO_BEARER,Rlc_info_am_config);
  //  UE_rrc_inst[ue_mod_idP].Srb1[eNB_index].Srb_info.Tx_buffer.payload_size=DEFAULT_MEAS_IND_SIZE+1;
  return(0);
}

//-----------------------------------------------------------------------------
int32_t
nr_rrc_ue_establish_drb(
    module_id_t       ue_mod_idP,
    frame_t           frameP,
    uint8_t           gNB_index,
    NR_DRB_ToAddMod_t *DRB_config
)
//-----------------------------------------------------------------------------
{
  // add descriptor from RRC PDU
  int oip_ifup = 0, ip_addr_offset3 = 0, ip_addr_offset4 = 0;
  /* avoid gcc warnings */
  (void)oip_ifup;
  (void)ip_addr_offset3;
  (void)ip_addr_offset4;
  LOG_I(NR_RRC,"[UE %d] Frame %d: processing RRCReconfiguration: reconfiguring DRB %ld\n",
        ue_mod_idP, frameP, DRB_config->drb_Identity);

//   if(!AMF_MODE_ENABLED) {
    ip_addr_offset3 = 0;
    ip_addr_offset4 = 1;
    LOG_I(OIP, "[UE %d] trying to bring up the OAI interface %d, IP X.Y.%d.%d\n", ue_mod_idP, ip_addr_offset3+ue_mod_idP,
          ip_addr_offset3+ue_mod_idP+1, ip_addr_offset4+ue_mod_idP+1);
    oip_ifup = nas_config(ip_addr_offset3+ue_mod_idP+1,   // interface_id
                        UE_NAS_USE_TUN?1:(ip_addr_offset3+ue_mod_idP+1), // third_octet
                        ip_addr_offset4+ue_mod_idP+1, // fourth_octet
                        "oip");                        // interface suffix (when using kernel module)

    if (oip_ifup == 0 && (!UE_NAS_USE_TUN)) { // interface is up --> send a config the DRB
      LOG_I(OIP, "[UE %d] Config the ue net interface %d to send/receive pkt on DRB %ld to/from the protocol stack\n",
            ue_mod_idP,
            ip_addr_offset3+ue_mod_idP,
            (long int)((gNB_index * NR_maxDRB) + DRB_config->drb_Identity));
      rb_conf_ipv4(0,//add
                   ue_mod_idP,//cx align with the UE index
                   ip_addr_offset3+ue_mod_idP,//inst num_enb+ue_index
                   (gNB_index * NR_maxDRB) + DRB_config->drb_Identity,//rb
                   0,//dscp
                   ipv4_address(ip_addr_offset3+ue_mod_idP+1, ip_addr_offset4+ue_mod_idP+1),//saddr
                   ipv4_address(ip_addr_offset3+ue_mod_idP+1, gNB_index+1));//daddr
      LOG_D(NR_RRC,"[UE %d] State = Attached (gNB %d)\n",ue_mod_idP,gNB_index);
    }
//   }

  return(0);
}

//-----------------------------------------------------------------------------
void
nr_rrc_ue_process_measConfig(
    const protocol_ctxt_t *const       ctxt_pP,
    const uint8_t                      gNB_index,
    NR_MeasConfig_t *const             measConfig
)
//-----------------------------------------------------------------------------
{
    int i;
    long ind;
    NR_MeasObjectToAddMod_t   *measObj        = NULL;
    NR_ReportConfigToAddMod_t *reportConfig   = NULL;

    if (measConfig->measObjectToRemoveList != NULL) {
        for (i = 0; i < measConfig->measObjectToRemoveList->list.count; i++) {
            ind = *measConfig->measObjectToRemoveList->list.array[i];
            free(NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]);
        }
    }

    if (measConfig->measObjectToAddModList != NULL) {
        LOG_I(NR_RRC, "Measurement Object List is present\n");
        for (i = 0; i < measConfig->measObjectToAddModList->list.count; i++) {
            measObj = measConfig->measObjectToAddModList->list.array[i];
            ind     = measConfig->measObjectToAddModList->list.array[i]->measObjectId;

            if (NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]) {
                LOG_D(NR_RRC, "Modifying measurement object %ld\n",ind);
                memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1],
                        (char *)measObj,
                        sizeof(NR_MeasObjectToAddMod_t));
            } else {
                LOG_I(NR_RRC, "Adding measurement object %ld\n", ind);

                if (measObj->measObject.present == NR_MeasObjectToAddMod__measObject_PR_measObjectNR) {
                    NR_UE_rrc_inst[ctxt_pP->module_id].MeasObj[gNB_index][ind-1]=measObj;
                }
            }
        }

        LOG_I(NR_RRC, "call rrc_mac_config_req \n");
        // rrc_mac_config_req_ue
    }

    if (measConfig->reportConfigToRemoveList != NULL) {
        for (i = 0; i < measConfig->reportConfigToRemoveList->list.count; i++) {
            ind = *measConfig->reportConfigToRemoveList->list.array[i];
            free(NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1]);
        }
    }

    if (measConfig->reportConfigToAddModList != NULL) {
        LOG_I(NR_RRC,"Report Configuration List is present\n");
        for (i = 0; i < measConfig->reportConfigToAddModList->list.count; i++) {
            ind          = measConfig->reportConfigToAddModList->list.array[i]->reportConfigId;
            reportConfig = measConfig->reportConfigToAddModList->list.array[i];

            if (NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1]) {
                LOG_I(NR_RRC, "Modifying Report Configuration %ld\n", ind-1);
                memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1],
                        (char *)measConfig->reportConfigToAddModList->list.array[i],
                        sizeof(NR_ReportConfigToAddMod_t));
            } else {
                LOG_D(RRC,"Adding Report Configuration %ld %p \n", ind-1, measConfig->reportConfigToAddModList->list.array[i]);
                if (reportConfig->reportConfig.present == NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR) {
                    NR_UE_rrc_inst[ctxt_pP->module_id].ReportConfig[gNB_index][ind-1] = measConfig->reportConfigToAddModList->list.array[i];
                }
            }
        }
    }

    if (measConfig->measIdToRemoveList != NULL) {
        for (i = 0; i < measConfig->measIdToRemoveList->list.count; i++) {
            ind = *measConfig->measIdToRemoveList->list.array[i];
            free(NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1]);
        }
    }

    if (measConfig->measIdToAddModList != NULL) {
        for (i = 0; i < measConfig->measIdToAddModList->list.count; i++) {
            ind = measConfig->measIdToAddModList->list.array[i]->measId;

            if (NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1]) {
                LOG_D(NR_RRC, "Modifying Measurement ID %ld\n",ind-1);
                memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1],
                    (char *)measConfig->measIdToAddModList->list.array[i],
                    sizeof(NR_MeasIdToAddMod_t));
            } else {
                LOG_D(NR_RRC, "Adding Measurement ID %ld %p\n", ind-1, measConfig->measIdToAddModList->list.array[i]);
                NR_UE_rrc_inst[ctxt_pP->module_id].MeasId[gNB_index][ind-1] = measConfig->measIdToAddModList->list.array[i];
            }
        }
    }

    if (measConfig->quantityConfig != NULL) {
        if (NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index]) {
            LOG_D(RRC,"Modifying Quantity Configuration \n");
            memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index],
                    (char *)measConfig->quantityConfig,
                    sizeof(NR_QuantityConfig_t));
        } else {
            LOG_D(NR_RRC, "Adding Quantity configuration\n");
            NR_UE_rrc_inst[ctxt_pP->module_id].QuantityConfig[gNB_index] = measConfig->quantityConfig;
        }
    }

    if (measConfig->measGapConfig != NULL) {
        if (NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index]) {
            memcpy((char *)NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index],
                    (char *)measConfig->measGapConfig,
                    sizeof(NR_MeasGapConfig_t));
        } else {
            NR_UE_rrc_inst[ctxt_pP->module_id].measGapConfig[gNB_index] = measConfig->measGapConfig;
        }
    }

    if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_ssb_RSRP) {
        NR_UE_rrc_inst[ctxt_pP->module_id].s_measure = measConfig->s_MeasureConfig->choice.ssb_RSRP;
    } else if (measConfig->s_MeasureConfig->present == NR_MeasConfig__s_MeasureConfig_PR_csi_RSRP) {
        NR_UE_rrc_inst[ctxt_pP->module_id].s_measure = measConfig->s_MeasureConfig->choice.csi_RSRP;
    }
}

//-----------------------------------------------------------------------------
void
nr_rrc_ue_process_radioBearerConfig(
    const protocol_ctxt_t *const       ctxt_pP,
    const uint8_t                      gNB_index,
    NR_RadioBearerConfig_t *const      radioBearerConfig
)
//-----------------------------------------------------------------------------
{
    long SRB_id, DRB_id;
    int i, cnt;

    if (radioBearerConfig->securityConfig != NULL) {
        if (*radioBearerConfig->securityConfig->keyToUse == NR_SecurityConfig__keyToUse_master) {
            NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm =
                radioBearerConfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm;
            NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm =
                *radioBearerConfig->securityConfig->securityAlgorithmConfig->integrityProtAlgorithm;
        }
    }

    if (radioBearerConfig->srb_ToAddModList != NULL) {
        uint8_t *kRRCenc = NULL;
        uint8_t *kRRCint = NULL;
        derive_key_rrc_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                        NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kRRCenc);
        derive_key_rrc_int(NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm,
                        NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kRRCint);
        // Refresh SRBs
        nr_rrc_pdcp_config_asn1_req(ctxt_pP,
                                    radioBearerConfig->srb_ToAddModList,
                                    NULL,
                                    NULL,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm |
                                    (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4),
                                    kRRCenc,
                                    kRRCint,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
        // Refresh SRBs
        nr_rrc_rlc_config_asn1_req(ctxt_pP,
                                    radioBearerConfig->srb_ToAddModList,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                    );

        for (cnt = 0; cnt < radioBearerConfig->srb_ToAddModList->list.count; cnt++) {
            SRB_id = radioBearerConfig->srb_ToAddModList->list.array[cnt]->srb_Identity;
            LOG_D(NR_RRC,"[UE %d]: Frame %d SRB config cnt %d (SRB%ld)\n", ctxt_pP->module_id, ctxt_pP->frame, cnt, SRB_id);
            if (SRB_id == 1) {
                if (NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index]) {
                    memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index],
                        radioBearerConfig->srb_ToAddModList->list.array[cnt], sizeof(NR_SRB_ToAddMod_t));
                } else {
                    NR_UE_rrc_inst[ctxt_pP->module_id].SRB1_config[gNB_index] = radioBearerConfig->srb_ToAddModList->list.array[cnt];
                    nr_rrc_ue_establish_srb1(ctxt_pP->module_id,
                                            ctxt_pP->frame,
                                            gNB_index,
                                            radioBearerConfig->srb_ToAddModList->list.array[cnt]);

                    LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB1 gNB %d) --->][MAC_UE][MOD %02d][]\n",
                        ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id);
                    // rrc_mac_config_req_ue
                }
            } else {
                if (NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index]) {
                    memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index],
                        radioBearerConfig->srb_ToAddModList->list.array[cnt], sizeof(NR_SRB_ToAddMod_t));
                } else {
                    NR_UE_rrc_inst[ctxt_pP->module_id].SRB2_config[gNB_index] = radioBearerConfig->srb_ToAddModList->list.array[cnt];
                    nr_rrc_ue_establish_srb2(ctxt_pP->module_id,
                                            ctxt_pP->frame,
                                            gNB_index,
                                            radioBearerConfig->srb_ToAddModList->list.array[cnt]);

                    LOG_I(NR_RRC, "[FRAME %05d][RRC_UE][MOD %02d][][--- MAC_CONFIG_REQ  (SRB2 gNB %d) --->][MAC_UE][MOD %02d][]\n",
                        ctxt_pP->frame, ctxt_pP->module_id, gNB_index, ctxt_pP->module_id);
                    // rrc_mac_config_req_ue
                }
            } // srb2
        }
    } // srb_ToAddModList

    // Establish DRBs if present
    if (radioBearerConfig->drb_ToAddModList != NULL) {
        if ((NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB == NULL) &&
            (radioBearerConfig->drb_ToAddModList->list.count >= 1)) {
            NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB = malloc(sizeof(rb_id_t));
            *NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB = radioBearerConfig->drb_ToAddModList->list.array[0]->drb_Identity;
        }

        for (cnt = 0; cnt < radioBearerConfig->drb_ToAddModList->list.count; cnt++) {
            DRB_id = radioBearerConfig->drb_ToAddModList->list.array[cnt]->drb_Identity;
            if (NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1]) {
                memcpy(NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1],
                        radioBearerConfig->drb_ToAddModList->list.array[cnt], sizeof(NR_DRB_ToAddMod_t));
            } else {
                LOG_D(NR_RRC, "Adding DRB %ld %p\n", DRB_id-1, radioBearerConfig->drb_ToAddModList->list.array[cnt]);
                NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1] = radioBearerConfig->drb_ToAddModList->list.array[cnt];
            }
        }

        uint8_t *kUPenc = NULL;
        derive_key_up_enc(NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm,
                        NR_UE_rrc_inst[ctxt_pP->module_id].kgnb, &kUPenc);
        MSC_LOG_TX_MESSAGE(
            MSC_RRC_UE,
            MSC_PDCP_UE,
            NULL,
            0,
            MSC_AS_TIME_FMT" CONFIG_REQ UE %x DRB (security %X)",
            MSC_AS_TIME_ARGS(ctxt_pP),
            ctxt_pP->rnti,
            NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm |
            (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4));

        // Refresh DRBs
        nr_rrc_pdcp_config_asn1_req(ctxt_pP,
                                    NULL,
                                    radioBearerConfig->drb_ToAddModList,
                                    NULL,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].cipheringAlgorithm |
                                    (NR_UE_rrc_inst[ctxt_pP->module_id].integrityProtAlgorithm << 4),
                                    NULL,
                                    NULL,
                                    kUPenc,
                                    NULL,
                                    NR_UE_rrc_inst[ctxt_pP->module_id].defaultDRB,
                                    NULL);
        // Refresh DRBs
        nr_rrc_rlc_config_asn1_req(ctxt_pP,
                                    NULL,
                                    radioBearerConfig->drb_ToAddModList,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
    } // drb_ToAddModList

    if (radioBearerConfig->drb_ToReleaseList != NULL) {
        for (i = 0; i < radioBearerConfig->drb_ToReleaseList->list.count; i++) {
            DRB_id = *radioBearerConfig->drb_ToReleaseList->list.array[i];
            free(NR_UE_rrc_inst[ctxt_pP->module_id].DRB_config[gNB_index][DRB_id-1]);
        }
    }

    NR_UE_rrc_inst[ctxt_pP->module_id].Info[gNB_index].State = NR_RRC_CONNECTED;
    LOG_I(NR_RRC,"[UE %d] State = NR_RRC_CONNECTED (eNB %d)\n", ctxt_pP->module_id, gNB_index);
}

//-----------------------------------------------------------------------------
void
rrc_ue_process_rrcReconfiguration(
  const protocol_ctxt_t *const  ctxt_pP,
  NR_RRCReconfiguration_t       *rrcReconfiguration,
  uint8_t                       gNB_index
)
//-----------------------------------------------------------------------------
{
    LOG_I(NR_RRC, "[UE %d] Frame %d: Receiving from SRB1 (DL-DCCH), Processing RRCReconfiguration (gNB %d)\n",
        ctxt_pP->module_id, ctxt_pP->frame, gNB_index);

    NR_RRCReconfiguration_IEs_t *ie = NULL;

    if (rrcReconfiguration->criticalExtensions.present 
                        == NR_RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration) {
        ie = rrcReconfiguration->criticalExtensions.choice.rrcReconfiguration;
        if (ie->measConfig != NULL) {
            LOG_I(NR_RRC, "Measurement Configuration is present\n");
            nr_rrc_ue_process_measConfig(ctxt_pP, gNB_index, ie->measConfig);
        }

        if (ie->radioBearerConfig != NULL) {
            LOG_I(NR_RRC, "radio Bearer Configuration is present\n");
            nr_rrc_ue_process_radioBearerConfig(ctxt_pP, gNB_index, ie->radioBearerConfig);
        }

        /* Check if there is dedicated NAS information to forward to NAS */
        if (ie->nonCriticalExtension->dedicatedNAS_MessageList != NULL) {
            int list_count;
            uint32_t pdu_length;
            uint8_t *pdu_buffer;
            MessageDef *msg_p;

            for (list_count = 0; list_count < ie->nonCriticalExtension->dedicatedNAS_MessageList->list.count; list_count++) {
                pdu_length = ie->nonCriticalExtension->dedicatedNAS_MessageList->list.array[list_count]->size;
                pdu_buffer = ie->nonCriticalExtension->dedicatedNAS_MessageList->list.array[list_count]->buf;
                msg_p = itti_alloc_new_message(TASK_RRC_UE, NAS_CONN_ESTABLI_CNF);
                NAS_CONN_ESTABLI_CNF(msg_p).errCode = AS_SUCCESS;
                NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.length = pdu_length;
                NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.data = pdu_buffer;
                itti_send_msg_to_task(TASK_NAS_UE, ctxt_pP->instance, msg_p);
            }

            free (ie->nonCriticalExtension->dedicatedNAS_MessageList);
        }
    }
}

//-----------------------------------------------------------------------------
void nr_rrc_ue_generate_RRCReconfigurationComplete( const protocol_ctxt_t *const ctxt_pP, const uint8_t gNB_index, const uint8_t Transaction_id ) {
  uint8_t buffer[32], size;
  size = do_NR_RRCReconfigurationComplete(ctxt_pP, buffer, Transaction_id);
  LOG_I(RRC,PROTOCOL_RRC_CTXT_UE_FMT" Logical Channel UL-DCCH (SRB1), Generating RRCReconfigurationComplete (bytes %d, gNB_index %d)\n",
        PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP), size, gNB_index);
  LOG_D(RLC,
        "[FRAME %05d][RRC_UE][INST %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCReconfigurationComplete to gNB %d MUI %d) --->][PDCP][INST %02d][RB %02d]\n",
        ctxt_pP->frame,
        UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
        size,
        gNB_index,
        nr_rrc_mui,
        UE_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id),
        DCCH);
  rrc_data_req_ue (
    ctxt_pP,
    DCCH,
    nr_rrc_mui++,
    SDU_CONFIRM_NO,
    size,
    buffer,
    PDCP_TRANSMISSION_MODE_CONTROL);
}

// from NR SRB1
//-----------------------------------------------------------------------------
int
nr_rrc_ue_decode_dcch(
  const protocol_ctxt_t *const ctxt_pP,
  const srb_id_t               Srb_id,
  const uint8_t         *const Buffer,
  const uint8_t                gNB_indexP
)
//-----------------------------------------------------------------------------
{
    asn_dec_rval_t                      dec_rval;
    NR_DL_DCCH_Message_t                *dl_dcch_msg  = NULL;

    if (Srb_id != 1) {
        LOG_E(NR_RRC,"[UE %d] Frame %d: Received message on DL-DCCH (SRB%ld), should not have ...\n",
            ctxt_pP->module_id, ctxt_pP->frame, Srb_id);
        return -1;
    } else {
        LOG_D(NR_RRC, "Received message on SRB%ld\n", Srb_id);
    }

    LOG_D(NR_RRC, "Decoding DL-DCCH Message\n");
    dec_rval = uper_decode( NULL,
                            &asn_DEF_NR_DL_DCCH_Message,
                            (void **)&dl_dcch_msg,
                            Buffer,
                            RRC_BUF_SIZE,
                            0,
                            0);

    if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
        LOG_E(NR_RRC, "Failed to decode DL-DCCH (%zu bytes)\n", dec_rval.consumed);
        return -1;
    }

    if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
        xer_fprint(stdout, &asn_DEF_NR_DL_DCCH_Message,(void *)dl_dcch_msg);
    }

    if (dl_dcch_msg->message.present == NR_DL_DCCH_MessageType_PR_c1) {
        switch (dl_dcch_msg->message.choice.c1->present) {
            case NR_DL_DCCH_MessageType__c1_PR_NOTHING:
                LOG_I(NR_RRC, "Received PR_NOTHING on DL-DCCH-Message\n");
                break;

            case NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
                rrc_ue_process_rrcReconfiguration(ctxt_pP,
                                                    dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration,
                                                    gNB_indexP);
                nr_rrc_ue_generate_RRCReconfigurationComplete(ctxt_pP,
                                            gNB_indexP,
                                            dl_dcch_msg->message.choice.c1->choice.rrcReconfiguration->rrc_TransactionIdentifier);
                break;

            case NR_DL_DCCH_MessageType__c1_PR_rrcResume:
            case NR_DL_DCCH_MessageType__c1_PR_rrcRelease:
            case NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment:
            case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer:
            case NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry:
            case NR_DL_DCCH_MessageType__c1_PR_mobilityFromNRCommand:
            case NR_DL_DCCH_MessageType__c1_PR_dlDedicatedMessageSegment_r16:
            case NR_DL_DCCH_MessageType__c1_PR_ueInformationRequest_r16:
            case NR_DL_DCCH_MessageType__c1_PR_dlInformationTransferMRDC_r16:
            case NR_DL_DCCH_MessageType__c1_PR_loggedMeasurementConfiguration_r16:
            case NR_DL_DCCH_MessageType__c1_PR_spare3:
            case NR_DL_DCCH_MessageType__c1_PR_spare2:
            case NR_DL_DCCH_MessageType__c1_PR_spare1:
                break;
            case NR_DL_DCCH_MessageType__c1_PR_securityModeCommand:
                break;
        }
    }
    return 0;
}

/*--------------------------------------------------*/
static void rrc_ue_generate_RRCSetupComplete(
    const protocol_ctxt_t *const ctxt_pP,
    const uint8_t gNB_index,
    const uint8_t Transaction_id,
    uint8_t sel_plmn_id){
        uint8_t buffer[100];
        uint8_t size;
        const char *nas_msg;
        int   nas_msg_length;
        /*
         if (EPC_MODE_ENABLED) {
            nas_msg         = (char *) UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.data;
            nas_msg_length  = UE_rrc_inst[ctxt_pP->module_id].initialNasMsg.length;
             } else {
            nas_msg         = nas_attach_req_imsi;
            nas_msg_length  = sizeof(nas_attach_req_imsi);
            }
        */
       size = do_RRCSetupComplete(ctxt_pP->module_id,buffer,Transaction_id,sel_plmn_id,nas_msg_length,nas_msg);
       LOG_I(RRC,"[UE %d][RAPROC] Frame %d : Logical Channel UL-DCCH (SRB1), Generating RRCConnectionSetupComplete (bytes%d, gNB %d)\n",
        ctxt_pP->module_id,ctxt_pP->frame, size, gNB_index);
       LOG_D(RLC,
            "[FRAME %05d][RRC_UE][MOD %02d][][--- PDCP_DATA_REQ/%d Bytes (RRCConnectionSetupComplete to gNB %d MUI %d) --->][PDCP][MOD %02d][RB %02d]\n",
            ctxt_pP->frame, ctxt_pP->module_id+NB_RN_INST, size, gNB_index, nr_rrc_mui, ctxt_pP->module_id+NB_eNB_INST, DCCH);
        // ctxt_pP_local.rnti = ctxt_pP->rnti;
        rrc_data_req_ue(
            ctxt_pP,
            DCCH,
            nr_rrc_mui++,
            SDU_CONFIRM_NO,
            size,
            buffer,
            PDCP_TRANSMISSION_MODE_CONTROL);
    }

