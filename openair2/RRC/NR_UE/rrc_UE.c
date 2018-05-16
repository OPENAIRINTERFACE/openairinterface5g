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

/*! \file rrc_UE.c
 * \brief rrc procedures for UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2011 - 2014
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr
 */

#define RRC_UE
#define RRC_UE_C


//  header files for RRC message for NR might be change to add prefix in from of the file name.
#include "assertions.h"
#include "hashtable.h"
#include "asn1_conversions.h"
#include "defs.h"
#include "PHY/TOOLS/dB_routines.h"
#include "extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#ifndef CELLULAR
#include "RRC/LITE/MESSAGES/asn1_msg.h"
#endif
#include "RRCConnectionRequest.h"
#include "RRCConnectionReconfiguration.h"
#include "UL-CCCH-Message.h"
#include "DL-CCCH-Message.h"
#include "UL-DCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "BCCH-DL-SCH-Message.h"
#include "PCCH-Message.h"
#if defined(Rel10) || defined(Rel14)
#include "MCCH-Message.h"
#endif
#include "MeasConfig.h"
#include "MeasGapConfig.h"
#include "MeasObjectEUTRA.h"
#include "TDD-Config.h"
#include "UECapabilityEnquiry.h"
#include "UE-CapabilityRequest.h"
#include "RRC/NAS/nas_config.h"
#include "RRC/NAS/rb_config.h"
#if ENABLE_RAL
#include "rrc_UE_ral.h"
#endif

#if defined(ENABLE_SECURITY)
# include "UTIL/OSA/osa_defs.h"
#endif


#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif



// from NR SRB3
uint8_t nr_rrc_ue_decode_dcch(
    const uint8_t *buffer,
    const uint32_t size
){
    //  uper_decode by nr R15 rrc_connection_reconfiguration
    
    NR_RRC_DL_DCCH_Message_t *nr_dl_dcch_msg = (NR_RRC_DL_DCCH_Message_t *)0;

    uper_decode(NULL,
                &asn_DEF_NR_RRC_DL_DCCH_Message,    //might be added prefix later
                (void**)&nr_dl_dcch_msg,
                (uint8_t *)buffer,
                size, 0, 0); 

    if(nr_dl_dcch_msg != NULL){
        switch(nr_dl_dcch_msg->message.present){            
            case DL_DCCH_MessageType_PR_c1:

                switch(nr_dl_dcch_msg->message.choice.c1.present){
                    case DL_DCCH_MessageType__c1_PR_rrcReconfiguration:
                        nr_rrc_ue_process_rrcReconfiguration(&nr_dl_dcch_msg->message.choice.c1.choice.rrcReconfiguration);
                        break;

                    case DL_DCCH_MessageType__c1_PR_NOTHING:
                    case DL_DCCH_MessageType__c1_PR_spare15:
                    case DL_DCCH_MessageType__c1_PR_spare14:
                    case DL_DCCH_MessageType__c1_PR_spare13:
                    case DL_DCCH_MessageType__c1_PR_spare12:
                    case DL_DCCH_MessageType__c1_PR_spare11:
                    case DL_DCCH_MessageType__c1_PR_spare10:
                    case DL_DCCH_MessageType__c1_PR_spare9:
                    case DL_DCCH_MessageType__c1_PR_spare8:
                    case DL_DCCH_MessageType__c1_PR_spare7:
                    case DL_DCCH_MessageType__c1_PR_spare6:
                    case DL_DCCH_MessageType__c1_PR_spare5:
                    case DL_DCCH_MessageType__c1_PR_spare4:
                    case DL_DCCH_MessageType__c1_PR_spare3:
                    case DL_DCCH_MessageType__c1_PR_spare2:
                    case DL_DCCH_MessageType__c1_PR_spare1:
                    default:
                        //  not support or unuse
                        break;
                }   
                break;
            case DL_DCCH_MessageType_PR_NOTHING:
            case DL_DCCH_MessageType_PR_messageClassExtension:
            default:
                //  not support or unuse
                break;
        }
        
        //  release memory allocation
        free(nr_dl_dcch_msg);
    }else{
        //  log..
    }

    return 0;

}


// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
//  TODO check to use this or downer one
uint8_t nr_rrc_ue_decode_rrcReconfiguration(
    const uint8_t *buffer,
    const uint32_t size
){
    RRCReconfiguration_t *rrcReconfiguration;

    //  decoding
    uper_decode(NULL,
                &asn_DEF_RRCReconfiguration,
                (void **)&rrcReconfiguration,
                (uint8_t *)buffer, 
                size);
    
    nr_rrc_ue_process_rrcReconfiguration(rrcReconfiguration);   //  after decoder

    free(rrcReconfiguration);

}
// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (encoded)
//  TODO check to use this or upper one
uint8_t nr_rrc_ue_decode_secondary_cellgroup_config(
    const uint8_t *buffer,
    const uint32_t size
){
    CellGroupConfig_t *cellGroupConfig = (CellGroupConfig_t *)0;

    uper_decode(NULL,
                &asn_DEF_CellGroupConfig,   //might be added prefix later
                (void **)&cellGroupConfig,
                (uint8_t *)buffer,
                size, 0, 0); 

     nr_rrc_ue_process_scg_config(cellGroupConfig);

     free(cellGroupConfig);
}


// from LTE-RRC DL-DCCH RRCConnectionReconfiguration nr-secondary-cell-group-config (decoded)
// RRCReconfiguration
uint8_t nr_rrc_ue_process_rrcReconfiguration(RRCReconfiguration_t *rrcReconfiguration){

    switch(rrcReconfiguration.criticalExtensions.present){
        case RRCReconfiguration__criticalExtensions_PR_rrcReconfiguration:
            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->radioBearerConfig != (RadioBearerConfig_t *)0){
                nr_rrc_ue_process_radio_bearer_config(rrcReconfiguration->radioBearerConfig);
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->secondaryCellGroup != (OCTET_STRING_t *)0){
                CellGroupConfig_t *cellGroupConfig = (CellGroupConfig_t *)0;
                // TODO check if this deocder is need for decode "SecondaryCellGroup" of use type "CellGroupConfig" directly
                uper_decode(NULL,
                            &asn_DEF_CellGroupConfig,   //might be added prefix later
                            (void **)&cellGroupConfig,
                            (uint8_t *)rrcReconfiguration->secondaryCellGroup->buffer,
                            rrcReconfiguration->secondaryCellGroup.size, 0, 0); 

                nr_rrc_ue_process_scg_config(cellGroupConfig);

                free(cellGroupConfig);
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->measConfig != (MeasConfig *)0){
                nr_rrc_ue_process_meas_config(rrcReconfiguration.criticalExtensions.rrcReconfiguration->measConfig);
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->lateNonCriticalExtension != (OCTET_STRING_t *)0){
                //  unuse now
            }

            if(rrcReconfiguration.criticalExtensions.rrcReconfiguration->nonCriticalExtension != (RRCReconfiguration_IEs__nonCriticalExtension *)0){
                // unuse now
            }
            break;
        case RRCReconfiguration__criticalExtensions_PR_NOTHING:
        case RRCReconfiguration__criticalExtensions_PR_criticalExtensionsFuture:
        default:
            break;
    }
    
    
    
    // process
}

uint8_t nr_rrc_ue_process_meas_config(MeasConfig_t *meas_config){
    //  copy into nr_rrc inst
    memcpy( (void *)NR_UE_rrc_inst->measConfig,
            (void *)meas_config,
            sizeof(MeasConfig_t));
    // process it
}
uint8_t nr_rrc_ue_process_scg_config(CellGroupConfig_t *cell_group_config){
    //  copy into nr_rrc inst  

    nr_ue_process_rlc_bearer_list();
    nr_ue_process_mac_cell_group_config();
    nr_ue_process_physical_cell_group_config();
    nr_ue_process_spcell_config();
    nr_ue_process_spcell_list();
 
    memcpy( (void *)NR_UE_rrc_inst->cellGroupConfig,
            (void *)cellGroupConfig,
            sizeof(cellGroupConfig_t));
    // process it
}
uint8_t nr_rrc_ue_process_radio_bearer_config(RadioBearerConfig_t *radio_bearer_config){
    //  copy into nr_rrc inst   
    memcpy( (void *)NR_UE_rrc_inst->radioBearerConfig,
            (void *)radio_bearer_config,
            sizeof(RadioBearerConfig_t));
    // process it
}


uint8_t openair_rrc_top_init_ue_nr(void){

    if(NB_NR_UE_INST > 0){
        NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)malloc(NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));
        memset(NR_UE_rrc_inst, 0, NB_NR_UE_INST * sizeof(NR_UE_RRC_INST_t));

        // fill UE-NR-Capability @ UE-CapabilityRAT-Container here.


    }else{
        NR_UE_rrc_inst = (NR_UE_RRC_INST_t *)0;
    }
}


uint8_t nr_ue_process_rlc_bearer_list(){
};

uint8_t nr_ue_process_mac_cell_group_config(){
};

uint8_t nr_ue_process_physical_cell_group_config(){
};

uint8_t nr_ue_process_spcell_config(){
};

uint8_t nr_ue_process_spcell_list(){
};


