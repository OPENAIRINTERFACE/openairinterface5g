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

#include "rrc_list.h"
#include "rrc_defs.h"
#include "rrc_proto.h"
#include "rrc_vars.h"
#include "mac_proto.h"

#include "executables/softmodem-common.h"


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

	if (get_softmodem_params()->phy_test==1) {
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
                    case NR_DL_DCCH_MessageType__c1_PR_spare7:
                    case NR_DL_DCCH_MessageType__c1_PR_spare6:
                    case NR_DL_DCCH_MessageType__c1_PR_spare5:
                    case NR_DL_DCCH_MessageType__c1_PR_spare4:
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
