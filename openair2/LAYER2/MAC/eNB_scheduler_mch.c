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

/*! \file eNB_scheduler_mch.c
 * \brief procedures related to eNB for the MCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2012 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */


#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"
#include "assertions.h"

#include "SIMULATION/TOOLS/sim.h"	// for taus

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1
#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

int8_t
get_mbsfn_sf_alloction(module_id_t module_idP, uint8_t CC_id,
		       uint8_t mbsfn_sync_area)
{
    // currently there is one-to-one mapping between sf allocation pattern and sync area
    if (mbsfn_sync_area > MAX_MBSFN_AREA) {
	LOG_W(MAC,
	      "[eNB %d] CC_id %d MBSFN synchronization area %d out of range\n ",
	      module_idP, CC_id, mbsfn_sync_area);
	return -1;
    }else if (RC.mac[module_idP]->
	       common_channels[CC_id].non_mbsfn_SubframeConfig
	       != NULL) {
	return mbsfn_sync_area;
    }else if (RC.mac[module_idP]->
	       common_channels[CC_id].mbsfn_SubframeConfig[mbsfn_sync_area]
	       != NULL) {
	return mbsfn_sync_area;
    } else {
	LOG_W(MAC,
	      "[eNB %d] CC_id %d MBSFN Subframe Config pattern %d not found \n ",
	      module_idP, CC_id, mbsfn_sync_area);
	return -1;
    }
}

static uint32_t bytes_in_buffer=0;
static uint32_t msi_pmch_stop=0;
rb_id_t mbms_rab_id=2047;//[8] = {2047,2047,2047,2047,2047,2047,2047,2047};
static uint32_t msi_sfs=0;


//MSI_ELEMENT * ptr =NULL;

static int check_CAS_sf(frame_t frameP,sub_frame_t subframeP){
   if( ((frameP&3)==0) && (subframeP == 0))
	return 1;	
   else 
   	return 0;
}
static int check_nonMBSFN_sf(frame_t frameP,COMMON_channels_t *cc,int sf){
   uint32_t non_mbsfn_SubframeConfig = (cc->non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | cc->non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]>>7);	
   if( frameP % (4<<cc->non_mbsfn_SubframeConfig->radioFrameAllocationPeriod_r14) == cc->non_mbsfn_SubframeConfig->radioFrameAllocationOffset_r14 ){
	return (non_mbsfn_SubframeConfig & (0x200>>(sf)))==(0x200 >> (sf));
   }
   return 0;
}


static int x=0;
static int mbms_mch_i=0;

int
schedule_MBMS_NFAPI(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
	      sub_frame_t subframeP)
{

    nfapi_dl_config_request_body_t *dl_req;
    //nfapi_dl_config_request_pdu_t *dl_config_pdu;
    
    int mcch_flag = 0, mtch_flag = 0, msi_flag = 0;
    int mbsfn_period = 0;	// 1<<(RC.mac[module_idP]->mbsfn_SubframeConfig[0]->radioframeAllocationPeriod);
    int mcch_period = 0;	//32<<(RC.mac[module_idP]->mbsfn_AreaInfo[0]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
    //TOCHECK mtch index here
    if(RC.mac[module_idP]->common_channels[CC_id].
              pmch_Config[0] == NULL )
	return 0;

    //TOCHECK mtch index here
    int mch_scheduling_period =
	8 << (RC.mac[module_idP]->common_channels[CC_id].
	      pmch_Config[0]->mch_SchedulingPeriod_r9);

    unsigned char mcch_sdu_length;
    unsigned char header_len_mcch = 0, header_len_msi =
	0, header_len_mtch = 0, header_len_mtch_temp =
	0, header_len_mcch_temp = 0, header_len_msi_temp = 0;
    int ii = 0, msi_pos = -1;
    int mcch_mcs = -1;
    int shifted_sf = 0;
    uint16_t TBS, padding = 0, post_padding = 0;
    mac_rlc_status_resp_t rlc_status;
    int num_mtch=0;
    int msi_length=0, i, k;

    unsigned char sdu_lcids[11], num_sdus = 0, offset = 0;
    uint16_t sdu_lengths[11], sdu_length_total = 0;
    unsigned char mch_buffer[MAX_DLSCH_PAYLOAD_BYTES];	// check the max value, this is for dlsch only
    int16_t j = -1;

    COMMON_channels_t *cc = &RC.mac[module_idP]->common_channels[CC_id];

    cc->MCH_pdu.Pdu_size = 0;

    for (i = 0; i < cc->num_active_mbsfn_area; i++) {
	// assume, that there is always a mapping
	if ((j = get_mbsfn_sf_alloction(module_idP, CC_id, i)) == -1) {
	    return 0;
	}

        if(cc->non_mbsfn_SubframeConfig){
            int alloc_offset=0;
            uint32_t period;

            uint32_t non_mbsfn_SubframeConfig = (cc->non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]<<1 | cc->non_mbsfn_SubframeConfig->subframeAllocation_r14.buf[0]>>7);	
            long mcch_offset        = cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9;

            period = 4<<cc->non_mbsfn_SubframeConfig->radioFrameAllocationPeriod_r14;
            alloc_offset = cc->non_mbsfn_SubframeConfig->radioFrameAllocationOffset_r14;
            mcch_period  = 32 << cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_RepetitionPeriod_r9;
         
            // get the real MCS value
            switch (cc->mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
        	case 0:
          	mcch_mcs = 2;
          	break;

        	case 1:
          	mcch_mcs = 7;
          	break;

        	case 2:
          	mcch_mcs = 13;
          	break;

        	case 3:
          	mcch_mcs = 19;
          	break;
            }
	   	  
            if (cc->pmch_Config[0]) {
        		mch_scheduling_period = 8 << cc->pmch_Config[0]->mch_SchedulingPeriod_r9;
            }

           LOG_D(MAC,"frameP %d subframe %d period %d alloc_offset %d mcch_mcs %d mcch_period %d mcch_offset %ld buf %x mch_scheduling_period %d\n",frameP, subframeP, period, alloc_offset,mcch_mcs, mcch_period, mcch_offset,(cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0]),mch_scheduling_period);
	//if( (frameP % (4 << cc->commonSF_AllocPeriod_r9) ) == 0 ){
	//	   if((subframeP==0)){
	//	   	x=0;
	//	   	mbms_mch_i=0;
	//	   }
	//}
            //if (frameP % (4 << cc->commonSF_AllocPeriod_r9 ) == 0) {
	    //       if((subframeP==0)){
	    //       	x=0;
	    //       	mbms_mch_i=0;
	    //       }
	    //}
            if (cc->pmch_Config[0]) {
              //  Find the first subframe in this MCH to transmit MSI
              if (frameP % 1 == 0) {
                if (frameP % mch_scheduling_period == 0) {
		   msi_pos=0;
		   if((frameP&3)==0)
			msi_pos++;
		   while((non_mbsfn_SubframeConfig & (0x100 >> msi_pos)) == (0x100>>msi_pos))
			msi_pos++;
		   mbms_mch_i=0;

		   if (subframeP==0) {
		   	x=0;
		   	mbms_mch_i=0;
		   }
		 
		}
              }
            }

	    if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9+shifted_sf){

			shifted_sf = check_nonMBSFN_sf(frameP,cc,subframeP)+check_CAS_sf(frameP,subframeP);
			msi_pos=subframeP+shifted_sf;
			if(shifted_sf==0)
				mbms_mch_i++;
			LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d shifted_sf %d\n",frameP, subframeP, msi_pos,mbms_mch_i,shifted_sf);
		}
	    }

            // Check if the subframe is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
            switch (subframeP) {
              case 0: 
          	    if (msi_pos == 0) {
            	      msi_flag = 1;
          	    }
                      mtch_flag = 1;
                break;
              case 1:
                    if (msi_pos == 1) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;
                break;

              case 2:
                    if (msi_pos == 2) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;

                break;

              case 3:
                    if (msi_pos == 3) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;
                break;

              case 4: 
                    if (msi_pos == 4) {
                      msi_flag = 1;
                    }
                      mtch_flag = 1;
                break;

              case 5: 
                    if (msi_pos == 5) {
                      msi_flag = 1;
                    }
                      mtch_flag = 1;
                break;

              case 6:
                    if (msi_pos == 6) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;
                break;

              case 7:
                    if (msi_pos == 7) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;
                break;

              case 8:
                    if (msi_pos == 8) {
                      msi_flag = 1;
                    }
                    if ((frameP % mcch_period == mcch_offset) && ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8)) {
                      mcch_flag = 1;
                    }
                    mtch_flag = 1;
                break;

              case 9: 
                    if (msi_pos == 9) {
                      msi_flag = 1;
                    }
                      mtch_flag = 1;
                break;
            }// end switch

            if ((msi_flag == 1) || (mcch_flag == 1) || (mtch_flag == 1)) {
                        LOG_D(MAC,
                              "[eNB %d] CC_id %d Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d\n",
                              module_idP, CC_id, frameP, subframeP, i, j,
                              msi_flag, mcch_flag, mtch_flag);
            }

	    //TODO quitar la subframe 0 del otro switch ... está mal interpretado ? 
           // if((frameP&3) == 0){
           //            mtch_flag=0;mcch_flag=0;msi_flag=0;
           // }

            if((frameP % period ) == alloc_offset){
               LOG_D(MAC,"non_mbsfn_SubframeConfig %x\n",non_mbsfn_SubframeConfig);
               switch(subframeP){
                  case 0:{
                       mtch_flag=0;mcch_flag=0;msi_flag=0;
                    }
                  break;
                  case 1:{
                  	if ((non_mbsfn_SubframeConfig & 0x100) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 2:{
                  	if ((non_mbsfn_SubframeConfig & 0x80) > 0)
                  	   {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 3:{
                  	if ((non_mbsfn_SubframeConfig & 0x40) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 4:{
                  	if ((non_mbsfn_SubframeConfig & 0x20) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 5:{
                  	if ((non_mbsfn_SubframeConfig & 0x10) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 6:{
                  	if ((non_mbsfn_SubframeConfig & 0x8) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 7:{
                  	if ((non_mbsfn_SubframeConfig & 0x4) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 8:{
                  	if ((non_mbsfn_SubframeConfig & 0x2) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                    }
                  break;
                  case 9:{
                  	if ((non_mbsfn_SubframeConfig & 0x1) > 0)
                       {mtch_flag=0;mcch_flag=0;msi_flag=0;}
                  }
                  break;
               }
            }
	    // sf allocation is non-overlapping
            if ((msi_flag == 1) || (mcch_flag == 1) || (mtch_flag == 1)) {
		  x++;
		  //if( (msi_flag!=1 && mcch_flag!=1) || (msi_flag!=1 && mcch_flag!=1 && mtch_flag!=1)  ){
		        	//x++;
		  //}
		 	
		  if(msi_flag==1){
		  	if ( (/*AJ*/ (/*V*/ ( /*U*/ (frameP %( 4 << cc->commonSF_AllocPeriod_r9)) ) / 8 ) % ((8 << cc->pmch_Config[mbms_mch_i]->mch_SchedulingPeriod_r9) / 8 ) ) != 0 ){
				msi_flag=0;
				LOG_D(MAC,"frameP %d subframeP %d reset(%d)\n",frameP, subframeP, mbms_mch_i);
			}
	        
		  }

	          LOG_D(MAC,"[eNB %d] CC_id %d Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d, x %d\n",
                  module_idP, CC_id, frameP, subframeP, i, j,
                   msi_flag, mcch_flag, mtch_flag,x);
                  break;
            }
	   

        }else if(cc->mbsfn_SubframeConfig[j]){

            mbsfn_period =
                1 << (cc->mbsfn_SubframeConfig[j]->radioframeAllocationPeriod);
            mcch_period =
                32 << (cc->mbsfn_AreaInfo[i]->
            	   mcch_Config_r9.mcch_RepetitionPeriod_r9);
            msi_pos = 0;
            ii = 0;
            LOG_D(MAC, "[eNB %d] CC_id %d Frame %d subframeP %d : Checking MBSFN Sync Area %d/%d with SF allocation %d/%d for MCCH and MTCH (mbsfn period %d, mcch period %d)\n",
                  module_idP, CC_id, frameP, subframeP, i,
                  cc->num_active_mbsfn_area, j, cc->num_sf_allocation_pattern,
                  mbsfn_period, mcch_period);

            switch (cc->mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
             case 0:
                 mcch_mcs = 2;
                 break;

             case 1:
                 mcch_mcs = 7;
                 break;

             case 2:
                 mcch_mcs = 13;
                 break;

             case 3:
                 mcch_mcs = 19;
                 break;
            }

            // 1st: Check the MBSFN subframes from SIB2 info (SF allocation pattern i, max 8 non-overlapping patterns exist)
            if (frameP % mbsfn_period == cc->mbsfn_SubframeConfig[j]->radioframeAllocationOffset) {	// MBSFN frameP
                if (cc->mbsfn_SubframeConfig[j]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {	// one-frameP format
            	     //  Find the first subframeP in this MCH to transmit MSI
            	     if (frameP % mch_scheduling_period == cc->mbsfn_SubframeConfig[j]->radioframeAllocationOffset) {
            	         while (ii == 0) {
            	     	ii = cc->
            	     	    mbsfn_SubframeConfig[j]->subframeAllocation.
            	     	    choice.oneFrame.buf[0] & (0x80 >> msi_pos);
            	     	msi_pos++;
            	         }
            	         LOG_D(MAC, "[eNB %d] CC_id %d Frame %d subframeP %d : sync area %d sf allocation pattern %d sf alloc %x msi pos is %d \n",
            	     	  module_idP, CC_id, frameP, subframeP, i, j,
            	     	  cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0],
            	     	  msi_pos);
	 	         if (subframeP==1) {
		        	x=0;
		        	mbms_mch_i=0;
	      	     	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		         }
            	     }
	       
            	     // Check if the subframeP is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
            	     switch (subframeP) {
            	     case 1:
            	         if (cc->tdd_Config == NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=1;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }
            	     	    if (msi_pos == 1) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ( (frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&& ( (cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1) ) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 2:
            	         if (cc->tdd_Config == NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF2) ==  MBSFN_FDD_SF2) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=2;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }

            	     	    if (msi_pos == 2) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ( (frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&& ((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF2) ==  MBSFN_FDD_SF2)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 3:
            	         if (cc->tdd_Config != NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF3) ==  MBSFN_TDD_SF3) {
            	     	    if (msi_pos == 1) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF3) ==  MBSFN_TDD_SF3)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         } else {	// FDD
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF3) ==  MBSFN_FDD_SF3) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=3;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }
            	     	    if (msi_pos == 3) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 4:
            	         if (cc->tdd_Config != NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4) {
            	     	    if (msi_pos == 2) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 6:
            	         if (cc->tdd_Config == NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF6) ==  MBSFN_FDD_SF6) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=4;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }
            	     	    if (msi_pos == 4) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF6) ==  MBSFN_FDD_SF6)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 7:
            	         if (cc->tdd_Config != NULL) {	// TDD
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF7) == MBSFN_TDD_SF7) {
            	     	    if (msi_pos == 3) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF7) ==  MBSFN_TDD_SF7)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         } else {	// FDD
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=5;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }

            	     	    if (msi_pos == 5) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 8:
            	         if (cc->tdd_Config != NULL) {	//TDD
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8) {
            	     	    if (msi_pos == 4) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         } else {	// FDD
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF8) ==  MBSFN_FDD_SF8) {
   		     	   if(cc->pmch_Config[mbms_mch_i+1]!=NULL){
		     	      if( x == cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9){
		     	 	msi_pos=6;
		     	 	mbms_mch_i++;
		     	 	LOG_D(MAC,"MSP, frameP %d subframeP %d msi_pos(%d) mbms_mch_i %d\n",frameP, subframeP, msi_pos,mbms_mch_i);
		     	      }
		     	    }
            	     	    if (msi_pos == 6) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     case 9:
            	         if (cc->tdd_Config != NULL) {
            	     	if ((cc->mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF9) ==  MBSFN_TDD_SF9) {
            	     	    if (msi_pos == 5) {
            	     		msi_flag = 1;
            	     	    }
            	     	    if ((frameP % mcch_period == cc->mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9)
            	     		&&((cc->mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF9) == MBSFN_TDD_SF9)) {
            	     		mcch_flag = 1;
            	     	    }
            	     	    mtch_flag = 1;
            	     	}
            	         }
            	         break;
            	     }	// end switch
            	     // sf allocation is non-overlapping
            	     if ((msi_flag == 1) || (mcch_flag == 1) || (mtch_flag == 1)) {
		     	  x++;
		         //if( (msi_flag!=1 && mcch_flag!=1) || (msi_flag!=1 && mcch_flag!=1 && mtch_flag!=1)  ){
		     		//x++;
		         //}
			  if(msi_flag==1){
			  	if ( (/*AJ*/ (/*V*/ ( /*U*/ (frameP %( 4 << cc->commonSF_AllocPeriod_r9)) ) / 8 ) % ((8 << cc->pmch_Config[mbms_mch_i]->mch_SchedulingPeriod_r9) / 8 ) ) != 0 ){
					msi_flag=0;
					LOG_D(MAC,"frameP %d subframeP %d reset(%d)\n",frameP, subframeP, mbms_mch_i);
				}
		        
			  }

            	         LOG_D(MAC,"[eNB %d] CC_id %d Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d x %d\n",
            	     	  module_idP, CC_id, frameP, subframeP, i, j,
            	     	  msi_flag, mcch_flag, mtch_flag,x);
            	         break;
            	     }
                } else {// four-frameP format
			 printf("Hola\n");
			 AssertFatal(1==0,"four-frameP format: not implemented yet\n");
                }
            }
        } // MBMS format
    }// end of for loop

    cc->msi_active = 0;
    cc->mcch_active = 0;
    cc->mtch_active = 0;

    // Calculate the mcs
    if ((msi_flag == 1) || (mcch_flag == 1)) {
	cc->MCH_pdu.mcs = mcch_mcs;
    } else if (mtch_flag == 1) {	// only MTCH in this subframeP
	cc->MCH_pdu.mcs = cc->pmch_Config[mbms_mch_i]->dataMCS_r9;
    }


     uint8_t num_sf_alloc=0;
    for( int iii=0; iii < 6; iii++)
       num_sf_alloc += ( 0x39 & (0x80>>iii)) == (0x80 >> iii);
    num_sf_alloc = 1;

    // 2nd: Create MSI, get MCCH from RRC and MTCHs from RLC

    // there is MSI (MCH Scheduling Info)
    uint16_t msi_control_element[29], *msi_ptr;
    // MSI buffer pointer
    char *buffer_pointer=NULL;
    if (msi_flag == 1 && cc->mbms_SessionList[mbms_mch_i]) {
      // Create MSI here
      msi_ptr = &msi_control_element[0];

      // Header for MTCHs
      num_mtch = cc->mbms_SessionList[mbms_mch_i]->list.count;

    	TBS =
	get_TBS_DL(cc->MCH_pdu.mcs, to_prb(cc->mib->message.dl_Bandwidth));
	//l=0;

	for (k = 0; k < num_mtch; k++) {	// loop for all session in this MCH (MCH[0]) at this moment

	    ((MSI_ELEMENT *) msi_ptr)->lcid = cc->mbms_SessionList[mbms_mch_i]->list.array[k]->logicalChannelIdentity_r9;	//mtch_lcid;

	    if( msi_sfs != 0 )
	    	msi_pmch_stop = msi_sfs-1;
	    else 
		msi_pmch_stop = msi_sfs;
	
    	    msi_pmch_stop = msi_sfs;

	    if(msi_pmch_stop>=num_sf_alloc /*&& msi_pmch_stop <=cc->pmch_Config[0]->sf_AllocEnd_r9*/)  {
	        ((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = (((msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9) >> 8) & 0x7f);
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = ((msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9) & 0xff);
		msi_pmch_stop = (msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9);
	    }else{
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0x7;	// last subframeP of this mtch (only one mtch now)
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = 0xFF; 
		msi_pmch_stop=0;
	    }
	    msi_ptr += sizeof(MSI_ELEMENT);
	}

	msi_length = msi_ptr - msi_control_element;

	if (msi_length < 128) {
	    header_len_msi = 2;
	} else {
	    header_len_msi = 3;
	}

	LOG_D(MAC, "[eNB %d] CC_id %d Frame %d : MSI->MCH, length of MSI is %d bytes TBS %d, bytes in buffer %d stop_sf_LSB %d msi_sfs %d cc->pmch_Config[0]->sf_AllocEnd_r9 %ld\n",
	      module_idP, CC_id, frameP, msi_length,TBS, bytes_in_buffer,msi_pmch_stop,msi_sfs,cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9);
	    
	msi_sfs = 0;

	// Store MSI data to mch_buffer[0]
	memcpy((char *) &mch_buffer[sdu_length_total],
	       msi_control_element, msi_length);

	buffer_pointer = (char *) &mch_buffer[sdu_length_total];

	sdu_lcids[num_sdus] = MCH_SCHDL_INFO;
	sdu_lengths[num_sdus] = msi_length;
	sdu_length_total += sdu_lengths[num_sdus];
	LOG_D(MAC, "[eNB %d] CC_id %d Create %d bytes for MSI\n",
	      module_idP, CC_id, sdu_lengths[num_sdus]);
	num_sdus++;
	cc->msi_active = 1;
    }
    // there is MCCH
    if (mcch_flag == 1) {
	//MCCH scheduling if !FeMBMS
	if(!cc->non_mbsfn_SubframeConfig){
          mcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, MCCH_COUNTING, 0xFFFC, 1, &cc->MCCH_pdu.payload[0], 
					 i);	// this is the mbsfn sync area index
	  if(mcch_sdu_length>0)
		LOG_I(MAC, "[eNB %d] CC_id %d Frame %d Subframe %d: Schedule MCCH MESSAGE COUNTING (area %d, sfAlloc %d)\n",
		  module_idP, CC_id, frameP, subframeP, i, j);

	  if (mcch_sdu_length > 0) {
              mcch_sdu_length+=1; //RLC ?
	      LOG_D(MAC, "[eNB %d] CC_id %d Frame %d subframeP %d : MCCH->MCH, Received %d bytes from RRC \n",
		    module_idP, CC_id, frameP, subframeP, mcch_sdu_length);

	      header_len_mcch = 2;

	      if (cc->tdd_Config != NULL) {
		  LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (TDD) for MCCH message %d bytes (mcs %d )\n",
			module_idP, CC_id, frameP, subframeP,
			mcch_sdu_length, mcch_mcs);
	      } else {
		  LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (FDD) for MCCH message %d bytes (mcs %d)\n",
			module_idP, CC_id, frameP, subframeP,
			mcch_sdu_length, mcch_mcs);
	      }

	      cc->mcch_active = 1;

	      memcpy((char *) &mch_buffer[sdu_length_total]+1,
		     &cc->MCCH_pdu.payload[0], mcch_sdu_length);
	      sdu_lcids[num_sdus] = MCCH_LCHANID;
	      sdu_lengths[num_sdus] = mcch_sdu_length;

	      if (sdu_lengths[num_sdus] > 128) {
		  header_len_mcch = 3;
	      }

	      sdu_length_total += sdu_lengths[num_sdus];
	      LOG_D(MAC,
		    "[eNB %d] CC_id %d Got %d bytes for MCCH from RRC \n",
		    module_idP, CC_id, sdu_lengths[num_sdus]);
	      num_sdus++;
	  }
	}

	//MCCH scheduling ... do it anyway
	{
	    LOG_I(MAC, "[eNB %d] CC_id %d Frame %d Subframe %d: Schedule MCCH MESSAGE (area %d, sfAlloc %d)\n",
	  	module_idP, CC_id, frameP, subframeP, i, j);

	    mcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, MCCH, 0xFFFC, 1, &cc->MCCH_pdu.payload[0], 
				       i);	// this is the mbsfn sync area index

	    if (mcch_sdu_length > 0) {
		mcch_sdu_length+=1; //RLC ?
		LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframeP %d : MCCH->MCH, Received %d bytes from RRC \n",
		      module_idP, CC_id, frameP, subframeP, mcch_sdu_length);

		header_len_mcch = 2;

		if (cc->tdd_Config != NULL) {
		    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (TDD) for MCCH message %d bytes (mcs %d )\n",
			  module_idP, CC_id, frameP, subframeP,
			  mcch_sdu_length, mcch_mcs);
		} else {
		    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (FDD) for MCCH message %d bytes (mcs %d)\n",
			  module_idP, CC_id, frameP, subframeP,
			  mcch_sdu_length, mcch_mcs);
		}

		cc->mcch_active = 1;

		memcpy((char *) &mch_buffer[sdu_length_total]+1,
		       &cc->MCCH_pdu.payload[0], mcch_sdu_length);
		sdu_lcids[num_sdus] = MCCH_LCHANID;
		sdu_lengths[num_sdus] = mcch_sdu_length;

		if (sdu_lengths[num_sdus] > 128) {
		    header_len_mcch = 3;
		}

		sdu_length_total += sdu_lengths[num_sdus];
		LOG_D(MAC,"[eNB %d] CC_id %d Got %d bytes for MCCH from RRC \n",
		      module_idP, CC_id, sdu_lengths[num_sdus]);
		num_sdus++;
	    }
	}
    }// mcch_flag


    TBS =
	get_TBS_DL(/*cc->pmch_Config[0]->dataMCS_r9*/cc->MCH_pdu.mcs, to_prb(cc->mib->message.dl_Bandwidth));
    // do not let mcch and mtch multiplexing when relaying is active
    // for sync area 1, so not transmit data
    //if ((i == 0) && ((RC.mac[module_idP]->MBMS_flag != multicast_relay) || (RC.mac[module_idP]->mcch_active==0))) {

    // there is MTCHs, loop if there are more than 1
    // BAd race condition: all this struct is filled by another thread, no mutex or any code to make it coherent
    if (mtch_flag == 1 && cc->mbms_SessionList[0] && cc->mbms_SessionList[0]->list.array[0]) {
      // Calculate TBS
      // get MTCH data from RLC (like for DTCH)
      LOG_D(MAC, "[eNB %d] CC_id %d Frame %d subframeP %d: Schedule MTCH (area %d, sfAlloc %d)\n", module_idP, CC_id, frameP, subframeP, i, j);

      header_len_mtch = 3;
      LOG_D(MAC,
            "[eNB %d], CC_id %d, Frame %d, MTCH->MCH, Checking RLC status (rab %d, tbs %d, len %d)\n",
            module_idP,
            CC_id,
            frameP,
            MTCH,
            TBS,
            TBS - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch);

      // TODO
      mbms_rab_id = cc->mbms_SessionList[0 /*mbms_mch_i*/]->list.array[0]->logicalChannelIdentity_r9;

      rlc_status = mac_rlc_status_ind(module_idP,
                                      0xfffd,
                                      frameP,
                                      subframeP,
                                      module_idP,
                                      ENB_FLAG_YES,
                                      MBMS_FLAG_YES,
                                      cc->mbms_SessionList[mbms_mch_i]->list.array[0]->logicalChannelIdentity_r9,
                                      // MTCH,
                                      0,
                                      0);

      bytes_in_buffer = rlc_status.bytes_in_buffer;

      // TOCHECK is this really neede?
      if (!(mcch_flag == 1 || msi_flag == 1))
        msi_sfs = rlc_status.bytes_in_buffer / (TBS - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch)
                  + (rlc_status.bytes_in_buffer % (TBS - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch) ? 1 : 0);

      uint16_t TBS_MTCH = get_TBS_DL(cc->pmch_Config[mbms_mch_i]->dataMCS_r9, to_prb(cc->mib->message.dl_Bandwidth));

      if (msi_flag == 1 && buffer_pointer != NULL) {
        //	msi_ptr = &msi_control_element[0];

        msi_pmch_stop =
            (rlc_status.bytes_in_buffer - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch) / (TBS_MTCH /*- header_len_mcch - header_len_msi -sdu_length_total*/ - header_len_mtch)
            + ((rlc_status.bytes_in_buffer - TBS - header_len_mcch - header_len_msi - sdu_length_total) % (TBS_MTCH /*- header_len_mcch - header_len_msi -sdu_length_total*/ - header_len_mtch) ? 0
                                                                                                                                                                                                : 0);

        for (k = 0; k < num_mtch; k++) { // loop for all session in this MCH (MCH[0]) at this moment
          msi_ptr = &msi_control_element[k];

          ((MSI_ELEMENT *)msi_ptr)->lcid = cc->mbms_SessionList[mbms_mch_i]->list.array[k]->logicalChannelIdentity_r9; // mtch_lcid;

          if (msi_pmch_stop > cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9)
            LOG_E(MAC, "e-MBMS Buffer Overflow\n");

          if (msi_pmch_stop >= num_sf_alloc /*&& msi_pmch_stop <=cc->pmch_Config[0]->sf_AllocEnd_r9*/) {
	          ((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = (((msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9) >> 8) & 0x7f);
	      	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = ((msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9) & 0xff);
	          msi_pmch_stop = (msi_pmch_stop <=cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9);
	      }else{
	      	((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0x7;	// last subframeP of this mtch (only one mtch now)
	      	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = 0xFF; 
	          msi_pmch_stop=0;
	      }
	      LOG_I(MAC,"frameP %d, subframeP %d LCID %d rlc_status.bytes_in_buffer %d stop_sf_LSB %d stop_sf_MSB %d msi_pmch_stop %d sf_AllocEnd_r9 %ld, msi_length %d\n",frameP,subframeP,((MSI_ELEMENT *) msi_ptr)->lcid,rlc_status.bytes_in_buffer,((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB,((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB, msi_pmch_stop,cc->pmch_Config[mbms_mch_i]->sf_AllocEnd_r9, msi_length);
	   }

	    memcpy((char*)buffer_pointer,
	       msi_control_element, msi_length);
	}

	LOG_D(MAC, "e-MBMS log channel %u frameP %d, subframeP %d,  rlc_status.bytes_in_buffer is %d TBS_MTCH %d pmch_stop %d msi_sfs %d\n",
	      MTCH, frameP, subframeP, rlc_status.bytes_in_buffer,TBS_MTCH,msi_pmch_stop,msi_sfs);

	//TODO not sure whether msi and mch MCH should be precluded ... keep in mind this konditions just in case if ((rlc_status.bytes_in_buffer > 0 &&  msi_pmch_stop > 0) && ((msi_flag!=1 || mcch_flag!=1))
	if ((rlc_status.bytes_in_buffer > 0 &&  msi_pmch_stop > 0) /*&& ((msi_flag!=1 || mcch_flag!=1))*/ /*|| (rlc_status.bytes_in_buffer > 0 && (msi_flag==1 || mcch_flag==1))*//*|| msi_sfs > cc->pmch_Config[0]->sf_AllocEnd_r9 */ /*msi_pmch_stop>=num_sf_alloc*/ ) {
	    LOG_I(MAC,"[eNB %d][MBMS USER-PLANE], CC_id %d, Frame %d, Subframe %d MTCH->MCH, Requesting %d bytes from RLC (header len mtch %d) rlc_status.bytes_in_buffer %d, LCID %ld\n",
		  module_idP, CC_id, frameP, subframeP,
		  TBS - header_len_mcch - header_len_msi -
		  sdu_length_total - header_len_mtch, header_len_mtch, rlc_status.bytes_in_buffer,cc->mbms_SessionList[mbms_mch_i]->list.array[0]->logicalChannelIdentity_r9);

      sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP, 0xfffd, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_YES,cc->mbms_SessionList[mbms_mch_i]->list.array[0]->logicalChannelIdentity_r9,
                                               TBS - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch,
						     (char *)
						     &mch_buffer[sdu_length_total]
                                ,0,
                                 0
                                 );
	    LOG_D(MAC,"[eNB %d][MBMS USER-PLANE] CC_id %d Got %d bytes for MTCH %d msi_pmch_stop %d msi_sfs %d sdu_lengths[num_sdus] %d\n",
		  module_idP, CC_id, sdu_lengths[num_sdus], MTCH,msi_pmch_stop,msi_sfs, sdu_lengths[num_sdus]);

	    cc->mtch_active = 1;

	    sdu_lcids[num_sdus] = cc->mbms_SessionList[mbms_mch_i]->list.array[0]->logicalChannelIdentity_r9/*MTCH*/;
	    sdu_length_total += sdu_lengths[num_sdus];

	    if (msi_pmch_stop != 0 && msi_flag !=1)
	    	msi_pmch_stop--;

	    if (sdu_lengths[num_sdus] < 128) {
		header_len_mtch = 2;
	    }

	    num_sdus++;
	} 
	else {
	    header_len_mtch = 0;
	}
    }

    // FINAL STEP: Prepare and multiplexe MSI, MCCH and MTCHs
    if ((sdu_length_total + header_len_msi + header_len_mcch + header_len_mtch) > 0) {

	// Adjust the last subheader
	header_len_mtch_temp = header_len_mtch;
	header_len_mcch_temp = header_len_mcch;
	header_len_msi_temp = header_len_msi;

	if (header_len_mtch > 0) {
	    header_len_mtch = 1;	// remove Length field in the  subheader for the last PDU
	} else if (header_len_mcch > 0) {
	    header_len_mcch = 1;
	} else {
	    header_len_msi = 1;
	}

	// Calculate the padding
	if ((TBS - header_len_mtch - header_len_mcch - header_len_msi -
	     sdu_length_total) < 0) {
	    LOG_E(MAC, "Error in building MAC PDU, TBS %d < PDU %d \n",
		  TBS,
		  header_len_mtch + header_len_mcch + header_len_msi +
		  sdu_length_total);
	    return 0;
	} else
	    if ((TBS - header_len_mtch - header_len_mcch - header_len_msi -
		 sdu_length_total) <= 2) {
	    padding =
		(TBS - header_len_mtch - header_len_mcch - header_len_msi -
		 sdu_length_total);
	    post_padding = 0;
	} else {		// using post_padding, give back the Length field of subheader  for the last PDU
	    padding = 0;

	    if (header_len_mtch > 0) {
		header_len_mtch = header_len_mtch_temp;
	    } else if (header_len_mcch > 0) {
		header_len_mcch = header_len_mcch_temp;
	    } else {
		header_len_msi = header_len_msi_temp;
	    }

	    post_padding =
		TBS - sdu_length_total - header_len_msi - header_len_mcch -
		header_len_mtch;
	}

	// Generate the MAC Header for MCH
	// here we use the function for DLSCH because DLSCH & MCH have the same Header structure
	offset = generate_dlsch_header((unsigned char *) cc->MCH_pdu.payload, num_sdus, sdu_lengths, sdu_lcids, 255,	// no drx
				       31,	// no timing advance
				       NULL,	// no contention res id
				       padding, post_padding);

	cc->MCH_pdu.Pdu_size = TBS;
	cc->MCH_pdu.sync_area = i;
	cc->MCH_pdu.msi_active = cc->msi_active;
	cc->MCH_pdu.mcch_active = cc->mcch_active;
	cc->MCH_pdu.mtch_active = cc->mtch_active;
	LOG_D(MAC, " MCS for this sf is %d (mcch active %d, mtch active %d)\n",
	      cc->MCH_pdu.mcs, cc->MCH_pdu.mcch_active,
	      cc->MCH_pdu.mtch_active);
	LOG_D(MAC, "[eNB %d][MBMS USER-PLANE ] CC_id %d Generate header : sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,padding %d,post_padding %d (mcs %d, TBS %d), header MTCH %d, header MCCH %d, header MSI %d\n",
	      module_idP, CC_id, sdu_length_total, num_sdus,
	      sdu_lengths[0], sdu_lcids[0], offset, padding, post_padding,
	      cc->MCH_pdu.mcs, TBS, header_len_mtch, header_len_mcch,
	      header_len_msi);
	// copy SDU to mch_pdu after the MAC Header
	memcpy(&cc->MCH_pdu.payload[offset], mch_buffer, sdu_length_total);

	// filling remainder of MCH with random data if necessery
	for (j = 0; j < (TBS - sdu_length_total - offset); j++) {
	    cc->MCH_pdu.payload[offset + sdu_length_total + j] =
		(char) (taus() & 0xff);
	}

	/* Tracing of PDU is done on UE side */
	//if (opt_enabled == 1) {
	trace_pdu(DIRECTION_DOWNLINK, (uint8_t *) cc->MCH_pdu.payload, TBS, module_idP, WS_M_RNTI , 0xfffd,	// M_RNTI = 6 in wirehsark
	          RC.mac[module_idP]->frame,
	          RC.mac[module_idP]->subframe, 0, 0);
	LOG_D(OPT, "[eNB %d][MCH] CC_id %d Frame %d : MAC PDU with size %d\n",
		  module_idP, CC_id, frameP, TBS);
	//}

       	eNB_MAC_INST *eNB = RC.mac[module_idP];
    	dl_req = &eNB->DL_req[CC_id].dl_config_request_body;
	dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
    	fill_nfapi_mch_config(
			dl_req,
			TBS,
			0,
			0xfffd,
			0,
			get_Qm(cc->MCH_pdu.mcs),
			cc->MCH_pdu.mcs,
			6000, //equal to RS power
			0 //mbsfn_area_id
			);

	eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
                                        (frameP * 10) + subframeP,
                                        TBS,
                                        eNB->pdu_index[CC_id],
                                        (uint8_t*)cc->MCH_pdu.payload);
	return 1;
    } else {
	cc->MCH_pdu.Pdu_size = 0;
	cc->MCH_pdu.sync_area = 0;
	cc->MCH_pdu.msi_active = 0;
	cc->MCH_pdu.mcch_active = 0;
	cc->MCH_pdu.mtch_active = 0;
	return 0;
    }
}
int
schedule_MBMS(module_id_t module_idP, uint8_t CC_id, frame_t frameP,
	      sub_frame_t subframeP)
{

    int mcch_flag = 0, mtch_flag = 0, msi_flag = 0;
    int mbsfn_period = 0;	// 1<<(RC.mac[module_idP]->mbsfn_SubframeConfig[0]->radioframeAllocationPeriod);
    int mcch_period = 0;	//32<<(RC.mac[module_idP]->mbsfn_AreaInfo[0]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
    if(RC.mac[module_idP]->common_channels[CC_id].
              pmch_Config[0] == NULL )

	return 0;


    int mch_scheduling_period =
	8 << (RC.mac[module_idP]->common_channels[CC_id].
	      pmch_Config[0]->mch_SchedulingPeriod_r9);
    unsigned char mcch_sdu_length;
    unsigned char header_len_mcch = 0, header_len_msi =
	0, header_len_mtch = 0, header_len_mtch_temp =
	0, header_len_mcch_temp = 0, header_len_msi_temp = 0;
    int ii = 0, msi_pos = 0;
    int mcch_mcs = -1;
    uint16_t TBS, padding = 0, post_padding = 0;
    mac_rlc_status_resp_t rlc_status;
    int num_mtch;
    int msi_length, i, k;
    //uint8_t l =0;
    unsigned char sdu_lcids[11], num_sdus = 0, offset = 0;
    uint16_t sdu_lengths[11], sdu_length_total = 0;
    unsigned char mch_buffer[MAX_DLSCH_PAYLOAD_BYTES];	// check the max value, this is for dlsch only
    int16_t j = -1;

    COMMON_channels_t *cc = &RC.mac[module_idP]->common_channels[CC_id];

    cc->MCH_pdu.Pdu_size = 0;

    for (i = 0; i < cc->num_active_mbsfn_area; i++) {
	// assume, that there is always a mapping
	if ((j = get_mbsfn_sf_alloction(module_idP, CC_id, i)) == -1) {
	    return 0;
	}

	mbsfn_period =
	    1 << (cc->mbsfn_SubframeConfig[j]->radioframeAllocationPeriod);
	mcch_period =
	    32 << (cc->mbsfn_AreaInfo[i]->
		   mcch_Config_r9.mcch_RepetitionPeriod_r9);
	msi_pos = 0;
	ii = 0;
	LOG_D(MAC,
	      "[eNB %d] CC_id %d Frame %d subframeP %d : Checking MBSFN Sync Area %d/%d with SF allocation %d/%d for MCCH and MTCH (mbsfn period %d, mcch period %d)\n",
	      module_idP, CC_id, frameP, subframeP, i,
	      cc->num_active_mbsfn_area, j, cc->num_sf_allocation_pattern,
	      mbsfn_period, mcch_period);


	switch (cc->mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
	case 0:
	    mcch_mcs = 2;
	    break;

	case 1:
	    mcch_mcs = 7;
	    break;

	case 2:
	    mcch_mcs = 13;
	    break;

	case 3:
	    mcch_mcs = 19;
	    break;
	}

	// 1st: Check the MBSFN subframes from SIB2 info (SF allocation pattern i, max 8 non-overlapping patterns exist)
	if (frameP % mbsfn_period == cc->mbsfn_SubframeConfig[j]->radioframeAllocationOffset) {	// MBSFN frameP
	    if (cc->mbsfn_SubframeConfig[j]->subframeAllocation.present == LTE_MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) {	// one-frameP format

		//  Find the first subframeP in this MCH to transmit MSI
		if (frameP % mch_scheduling_period ==
		    cc->mbsfn_SubframeConfig[j]->
		    radioframeAllocationOffset) {
		    while (ii == 0) {
			ii = cc->
			    mbsfn_SubframeConfig[j]->subframeAllocation.
			    choice.oneFrame.buf[0] & (0x80 >> msi_pos);
			msi_pos++;
		    }

		    LOG_D(MAC,
			  "[eNB %d] CC_id %d Frame %d subframeP %d : sync area %d sf allocation pattern %d sf alloc %x msi pos is %d \n",
			  module_idP, CC_id, frameP, subframeP, i, j,
			  cc->mbsfn_SubframeConfig[j]->
			  subframeAllocation.choice.oneFrame.buf[0],
			  msi_pos);
		}
		// Check if the subframeP is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
		switch (subframeP) {
		case 1:
		    if (cc->tdd_Config == NULL) {
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF1) ==
			    MBSFN_FDD_SF1) {
			    if (msi_pos == 1) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF1) ==
				 MBSFN_FDD_SF1)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 2:
		    if (cc->tdd_Config == NULL) {
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF2) ==
			    MBSFN_FDD_SF2) {
			    if (msi_pos == 2) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF2) ==
				 MBSFN_FDD_SF2)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 3:
		    if (cc->tdd_Config != NULL) {	// TDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_TDD_SF3) ==
			    MBSFN_TDD_SF3) {
			    if (msi_pos == 1) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_TDD_SF3) ==
				 MBSFN_TDD_SF3)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    } else {	// FDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF3) ==
			    MBSFN_FDD_SF3) {
			    if (msi_pos == 3) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF3) ==
				 MBSFN_FDD_SF3)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 4:
		    if (cc->tdd_Config != NULL) {
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_TDD_SF4) ==
			    MBSFN_TDD_SF4) {
			    if (msi_pos == 2) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_TDD_SF4) ==
				 MBSFN_TDD_SF4)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 6:
		    if (cc->tdd_Config == NULL) {
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF6) ==
			    MBSFN_FDD_SF6) {
			    if (msi_pos == 4) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF6) ==
				 MBSFN_FDD_SF6)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 7:
		    if (cc->tdd_Config != NULL) {	// TDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_TDD_SF7) ==
			    MBSFN_TDD_SF7) {
			    if (msi_pos == 3) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_TDD_SF7) ==
				 MBSFN_TDD_SF7)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    } else {	// FDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF7) ==
			    MBSFN_FDD_SF7) {
			    if (msi_pos == 5) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF7) ==
				 MBSFN_FDD_SF7)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 8:
		    if (cc->tdd_Config != NULL) {	//TDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_TDD_SF8) ==
			    MBSFN_TDD_SF8) {
			    if (msi_pos == 4) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_TDD_SF8) ==
				 MBSFN_TDD_SF8)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    } else {	// FDD
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_FDD_SF8) ==
			    MBSFN_FDD_SF8) {
			    if (msi_pos == 6) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_FDD_SF8) ==
				 MBSFN_FDD_SF8)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;

		case 9:
		    if (cc->tdd_Config != NULL) {
			if ((cc->
			     mbsfn_SubframeConfig[j]->subframeAllocation.
			     choice.oneFrame.buf[0] & MBSFN_TDD_SF9) ==
			    MBSFN_TDD_SF9) {
			    if (msi_pos == 5) {
				msi_flag = 1;
			    }

			    if ((frameP % mcch_period ==
				 cc->mbsfn_AreaInfo[i]->
				 mcch_Config_r9.mcch_Offset_r9)
				&&
				((cc->mbsfn_AreaInfo[i]->
				  mcch_Config_r9.sf_AllocInfo_r9.
				  buf[0] & MBSFN_TDD_SF9) ==
				 MBSFN_TDD_SF9)) {
				mcch_flag = 1;
			    }

			    mtch_flag = 1;
			}
		    }

		    break;
		}		// end switch

		// sf allocation is non-overlapping
		if ((msi_flag == 1) || (mcch_flag == 1)
		    || (mtch_flag == 1)) {
		    LOG_D(MAC,
			  "[eNB %d] CC_id %d Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d\n",
			  module_idP, CC_id, frameP, subframeP, i, j,
			  msi_flag, mcch_flag, mtch_flag);
		    break;
		}
	    } else {		// four-frameP format
	    }
	}
    }				// end of for loop

    cc->msi_active = 0;
    cc->mcch_active = 0;
    cc->mtch_active = 0;

    // Calculate the mcs
    if ((msi_flag == 1) || (mcch_flag == 1)) {
	cc->MCH_pdu.mcs = mcch_mcs;
    } else if (mtch_flag == 1) {	// only MTCH in this subframeP
	cc->MCH_pdu.mcs = cc->pmch_Config[0]->dataMCS_r9;
    }


     uint8_t num_sf_alloc=0;
    for( int iii=0; iii < 6; iii++)
       num_sf_alloc += ( 0x39 & (0x80>>iii)) == (0x80 >> iii);
    num_sf_alloc = 1;

    // 2nd: Create MSI, get MCCH from RRC and MTCHs from RLC

    // there is MSI (MCH Scheduling Info)
    if (msi_flag == 1) {
	// Create MSI here
	uint16_t msi_control_element[29], *msi_ptr;

	msi_ptr = &msi_control_element[0];
	//((MSI_ELEMENT *) msi_ptr)->lcid = MCCH_LCHANID;	//MCCH

	//if (mcch_flag == 1) {
	//    ((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0;
	//    ((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = 0;
	//} else {		// no mcch for this MSP
	//    ((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0x7;	// stop value is 2047
	//    ((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = 0xff;
	//}

	//msi_ptr += sizeof(MSI_ELEMENT);

	//Header for MTCHs
	num_mtch = cc->mbms_SessionList[0]->list.count;

    	TBS =
	get_TBS_DL(cc->MCH_pdu.mcs, to_prb(cc->mib->message.dl_Bandwidth));
	//l=0;

	for (k = 0; k < num_mtch; k++) {	// loop for all session in this MCH (MCH[0]) at this moment

	    ((MSI_ELEMENT *) msi_ptr)->lcid = cc->mbms_SessionList[0]->list.array[k]->logicalChannelIdentity_r9;	//mtch_lcid;

	    if( msi_sfs != 0 )
	    	msi_pmch_stop = msi_sfs-1;
	    else 
		msi_pmch_stop = msi_sfs;

	    if( msi_pmch_stop > cc->pmch_Config[0]->sf_AllocEnd_r9)
		   LOG_W(MAC,"e-MBMS Buffer Overflow\n"); 

	    if(msi_pmch_stop>=num_sf_alloc /*&& msi_pmch_stop <=cc->pmch_Config[0]->sf_AllocEnd_r9*/)  {
	        ((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0;	// last subframeP of this mtch (only one mtch now) & stop_sf limited to 256
	    	//((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = msi_pmch_stop;
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = (msi_pmch_stop <=cc->pmch_Config[0]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[0]->sf_AllocEnd_r9);
		msi_pmch_stop = (msi_pmch_stop <=cc->pmch_Config[0]->sf_AllocEnd_r9 ? msi_pmch_stop: cc->pmch_Config[0]->sf_AllocEnd_r9);
	    }else{
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_MSB = 0x7;	// last subframeP of this mtch (only one mtch now)
	    	((MSI_ELEMENT *) msi_ptr)->stop_sf_LSB = 0xFF; 
		msi_pmch_stop=0;
	    }

	   
	   
	    msi_ptr += sizeof(MSI_ELEMENT);
	}

	msi_length = msi_ptr - msi_control_element;

	if (msi_length < 128) {
	    header_len_msi = 2;
	} else {
	    header_len_msi = 3;
	}

	LOG_W(MAC,
	      "[eNB %d] CC_id %d Frame %d : MSI->MCH, length of MSI is %d bytes TBS %d, bytes in buffer %d stop_sf_LSB %d msi_sfs %d cc->pmch_Config[0]->sf_AllocEnd_r9 %ld\n",
	      module_idP, CC_id, frameP, msi_length,TBS, bytes_in_buffer,msi_pmch_stop,msi_sfs,cc->pmch_Config[0]->sf_AllocEnd_r9);
	    
	msi_sfs = 0;

	//LOG_D(MAC,"Scheduler: MSI is transmitted in this subframeP \n" );

	//   LOG_D(MAC,"Scheduler: MSI length is %d bytes\n",msi_length);
	// Store MSI data to mch_buffer[0]
	memcpy((char *) &mch_buffer[sdu_length_total],
	       msi_control_element, msi_length);

	sdu_lcids[num_sdus] = MCH_SCHDL_INFO;
	sdu_lengths[num_sdus] = msi_length;
	sdu_length_total += sdu_lengths[num_sdus];
	LOG_D(MAC, "[eNB %d] CC_id %d Create %d bytes for MSI\n",
	      module_idP, CC_id, sdu_lengths[num_sdus]);
	num_sdus++;
	cc->msi_active = 1;
    }
    // there is MCCH
    if (mcch_flag == 1) {
	LOG_D(MAC,
	      "[eNB %d] CC_id %d Frame %d Subframe %d: Schedule MCCH MESSAGE (area %d, sfAlloc %d)\n",
	      module_idP, CC_id, frameP, subframeP, i, j);

	mcch_sdu_length = mac_rrc_data_req(module_idP, CC_id, frameP, MCCH, 0xFFFC, 1, &cc->MCCH_pdu.payload[0], 
					   i);	// this is the mbsfn sync area index

        mcch_sdu_length+=1; //RLC ?

	if (mcch_sdu_length > 0) {
	    LOG_D(MAC,
		  "[eNB %d] CC_id %d Frame %d subframeP %d : MCCH->MCH, Received %d bytes from RRC \n",
		  module_idP, CC_id, frameP, subframeP, mcch_sdu_length);

	    header_len_mcch = 2;

	    if (cc->tdd_Config != NULL) {
		LOG_D(MAC,
		      "[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (TDD) for MCCH message %d bytes (mcs %d )\n",
		      module_idP, CC_id, frameP, subframeP,
		      mcch_sdu_length, mcch_mcs);
	    } else {
		LOG_D(MAC,
		      "[eNB %d] CC_id %d Frame %d subframeP %d: Scheduling MCCH->MCH (FDD) for MCCH message %d bytes (mcs %d)\n",
		      module_idP, CC_id, frameP, subframeP,
		      mcch_sdu_length, mcch_mcs);
	    }

	    cc->mcch_active = 1;

	    memcpy((char *) &mch_buffer[sdu_length_total]+1,
		   &cc->MCCH_pdu.payload[0], mcch_sdu_length);
	    sdu_lcids[num_sdus] = MCCH_LCHANID;
	    sdu_lengths[num_sdus] = mcch_sdu_length;

	    if (sdu_lengths[num_sdus] > 128) {
		header_len_mcch = 3;
	    }

	    sdu_length_total += sdu_lengths[num_sdus];
	    LOG_D(MAC,
		  "[eNB %d] CC_id %d Got %d bytes for MCCH from RRC \n",
		  module_idP, CC_id, sdu_lengths[num_sdus]);
	    num_sdus++;
	}
    }

    TBS =
	get_TBS_DL(/*cc->pmch_Config[0]->dataMCS_r9*/cc->MCH_pdu.mcs, to_prb(cc->mib->message.dl_Bandwidth));
    // do not let mcch and mtch multiplexing when relaying is active
    // for sync area 1, so not transmit data
    //if ((i == 0) && ((RC.mac[module_idP]->MBMS_flag != multicast_relay) || (RC.mac[module_idP]->mcch_active==0))) {

    // there is MTCHs, loop if there are more than 1
    if (mtch_flag == 1 ) {
	// Calculate TBS
	/* if ((msi_flag==1) || (mcch_flag==1)) {
	   TBS = mac_xface->get_TBS(mcch_mcs, mac_xface->frame_parms->N_RB_DL);
	   }
	   else { // only MTCH in this subframeP
	   TBS = mac_xface->get_TBS(RC.mac[module_idP]->pmch_Config[0]->dataMCS_r9, mac_xface->frame_parms->N_RB_DL);
	   }

	   // get MTCH data from RLC (like for DTCH)
	   LOG_D(MAC,"[eNB %d] CC_id %d Frame %d subframe %d: Schedule MTCH (area %d, sfAlloc %d)\n",Mod_id,CC_id,frame,subframe,i,j);

	   header_len_mtch = 3;
	   LOG_D(MAC,"[eNB %d], CC_id %d, Frame %d, MTCH->MCH, Checking RLC status (rab %d, tbs %d, len %d)\n",
	   Mod_id,CC_id,frame,MTCH,TBS,
	   TBS-header_len_mcch-header_len_msi-sdu_length_total-header_len_mtch);

	   rlc_status = mac_rlc_status_ind(Mod_id,frame,1,RLC_MBMS_YES,MTCH+ (maxDRB + 3) * MAX_MOBILES_PER_RG,
	   TBS-header_len_mcch-header_len_msi-sdu_length_total-header_len_mtch);
	   printf("frame %d, subframe %d,  rlc_status.bytes_in_buffer is %d\n",frame,subframe, rlc_status.bytes_in_buffer);

	 */

	// get MTCH data from RLC (like for DTCH)
	LOG_D(MAC,
	      "[eNB %d] CC_id %d Frame %d subframeP %d: Schedule MTCH (area %d, sfAlloc %d)\n",
	      module_idP, CC_id, frameP, subframeP, i, j);

	header_len_mtch = 3;
	LOG_D(MAC,
	      "[eNB %d], CC_id %d, Frame %d, MTCH->MCH, Checking RLC status (rab %d, tbs %d, len %d)\n",
	      module_idP, CC_id, frameP, MTCH, TBS,
	      TBS - header_len_mcch - header_len_msi - sdu_length_total -
	      header_len_mtch);

	mbms_rab_id = cc->mbms_SessionList[0]->list.array[0]->logicalChannelIdentity_r9;

	rlc_status =
	    mac_rlc_status_ind(module_idP, 0xfffd, frameP, subframeP,
			       module_idP, ENB_FLAG_YES, MBMS_FLAG_YES,
				cc->mbms_SessionList[0]->list.array[0]->logicalChannelIdentity_r9,
			       //MTCH,
                                     0, 0
                                    );
	bytes_in_buffer = rlc_status.bytes_in_buffer;


	msi_sfs = rlc_status.bytes_in_buffer/TBS+(rlc_status.bytes_in_buffer%TBS?1:0);

	LOG_D(MAC,
	      "e-MBMS log channel %u frameP %d, subframeP %d,  rlc_status.bytes_in_buffer is %d TBS %d pmch_stop %d msi_sfs %d\n",
	      MTCH, frameP, subframeP, rlc_status.bytes_in_buffer,TBS,msi_pmch_stop,msi_sfs);

	if (rlc_status.bytes_in_buffer > 0 ||  msi_pmch_stop > 0  /*msi_pmch_stop>=num_sf_alloc*/ ) {
	    //if(rlc_status.bytes_in_buffer > 0){
	    LOG_I(MAC,
		  "[eNB %d][MBMS USER-PLANE], CC_id %d, Frame %d, MTCH->MCH, Requesting %d bytes from RLC (header len mtch %d) rlc_status.bytes_in_buffer %d\n",
		  module_idP, CC_id, frameP,
		  TBS - header_len_mcch - header_len_msi -
		  sdu_length_total - header_len_mtch, header_len_mtch, rlc_status.bytes_in_buffer);

      sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP, 0xfffd, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_YES,cc->mbms_SessionList[0]->list.array[0]->logicalChannelIdentity_r9,
                                               TBS - header_len_mcch - header_len_msi - sdu_length_total - header_len_mtch,
						     (char *)
						     &mch_buffer[sdu_length_total]
                                ,0,
                                 0
                                 );
	
	    //sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,frameP, MBMS_FLAG_NO,  MTCH+(MAX_NUM_RB*(MAX_MOBILES_PER_ENB+1)), (char*)&mch_buffer[sdu_length_total]);
	    LOG_I(MAC,
		  "[eNB %d][MBMS USER-PLANE] CC_id %d Got %d bytes for MTCH %d msi_pmch_stop %d msi_sfs %d sdu_lengths[num_sdus] %d\n",
		  module_idP, CC_id, sdu_lengths[num_sdus], MTCH,msi_pmch_stop,msi_sfs, sdu_lengths[num_sdus]);
	    cc->mtch_active = 1;
	    sdu_lcids[num_sdus] = cc->mbms_SessionList[0]->list.array[0]->logicalChannelIdentity_r9/*MTCH*/;
	    sdu_length_total += sdu_lengths[num_sdus];

	    if (msi_pmch_stop != 0)
	    	msi_pmch_stop--;


	    if (sdu_lengths[num_sdus] < 128) {
		header_len_mtch = 2;
	    }

	    num_sdus++;
	  //}
	} 
	else {
	//    LOG_E(MAC,
	//	  "[eNB %d][MBMS USER-PLANE] CC_id %d Got %d bytes for MTCH %d msi_pmch_stop %d msi_buffer %d msi_sfs %ld msi_buffer_act %ld  sdu_lengths[num_sdus] %d\n",
	//	  module_idP, CC_id, sdu_lengths[num_sdus], MTCH,msi_pmch_stop,msi_sfs,msi_buffer_act, sdu_lengths[num_sdus]);

	    header_len_mtch = 0;
	}
    }
    //  }

    // FINAL STEP: Prepare and multiplexe MSI, MCCH and MTCHs
    if ((sdu_length_total + header_len_msi + header_len_mcch +
	 header_len_mtch) > 0) {
	// Adjust the last subheader
	/*                                 if ((msi_flag==1) || (mcch_flag==1)) {
	   RC.mac[module_idP]->MCH_pdu.mcs = mcch_mcs;
	   }
	   else if (mtch_flag == 1) { // only MTCH in this subframeP
	   RC.mac[module_idP]->MCH_pdu.mcs = RC.mac[module_idP]->pmch_Config[0]->dataMCS_r9;
	   }
	 */
	header_len_mtch_temp = header_len_mtch;
	header_len_mcch_temp = header_len_mcch;
	header_len_msi_temp = header_len_msi;

	if (header_len_mtch > 0) {
	    header_len_mtch = 1;	// remove Length field in the  subheader for the last PDU
	} else if (header_len_mcch > 0) {
	    header_len_mcch = 1;
	} else {
	    header_len_msi = 1;
	}

	// Calculate the padding
	if ((TBS - header_len_mtch - header_len_mcch - header_len_msi -
	     sdu_length_total) < 0) {
	    LOG_E(MAC, "Error in building MAC PDU, TBS %d < PDU %d \n",
		  TBS,
		  header_len_mtch + header_len_mcch + header_len_msi +
		  sdu_length_total);
	    return 0;
	} else
	    if ((TBS - header_len_mtch - header_len_mcch - header_len_msi -
		 sdu_length_total) <= 2) {
	    padding =
		(TBS - header_len_mtch - header_len_mcch - header_len_msi -
		 sdu_length_total);
	    post_padding = 0;
	} else {		// using post_padding, give back the Length field of subheader  for the last PDU
	    padding = 0;

	    if (header_len_mtch > 0) {
		header_len_mtch = header_len_mtch_temp;
	    } else if (header_len_mcch > 0) {
		header_len_mcch = header_len_mcch_temp;
	    } else {
		header_len_msi = header_len_msi_temp;
	    }

	    post_padding =
		TBS - sdu_length_total - header_len_msi - header_len_mcch -
		header_len_mtch;
	}

	// Generate the MAC Header for MCH
	// here we use the function for DLSCH because DLSCH & MCH have the same Header structure
	offset = generate_dlsch_header((unsigned char *) cc->MCH_pdu.payload, num_sdus, sdu_lengths, sdu_lcids, 255,	// no drx
				       31,	// no timing advance
				       NULL,	// no contention res id
				       padding, post_padding);

	cc->MCH_pdu.Pdu_size = TBS;
	cc->MCH_pdu.sync_area = i;
	cc->MCH_pdu.msi_active = cc->msi_active;
	cc->MCH_pdu.mcch_active = cc->mcch_active;
	cc->MCH_pdu.mtch_active = cc->mtch_active;
	LOG_D(MAC,
	      " MCS for this sf is %d (mcch active %d, mtch active %d)\n",
	      cc->MCH_pdu.mcs, cc->MCH_pdu.mcch_active,
	      cc->MCH_pdu.mtch_active);
	LOG_D(MAC,
	      "[eNB %d][MBMS USER-PLANE ] CC_id %d Generate header : sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,padding %d,post_padding %d (mcs %d, TBS %d), header MTCH %d, header MCCH %d, header MSI %d\n",
	      module_idP, CC_id, sdu_length_total, num_sdus,
	      sdu_lengths[0], sdu_lcids[0], offset, padding, post_padding,
	      cc->MCH_pdu.mcs, TBS, header_len_mtch, header_len_mcch,
	      header_len_msi);
	// copy SDU to mch_pdu after the MAC Header
	memcpy(&cc->MCH_pdu.payload[offset], mch_buffer, sdu_length_total);

	// filling remainder of MCH with random data if necessery
	for (j = 0; j < (TBS - sdu_length_total - offset); j++) {
	    cc->MCH_pdu.payload[offset + sdu_length_total + j] =
		(char) (taus() & 0xff);
	}

	/* Tracing of PDU is done on UE side */
	//if (opt_enabled == 1) {
	    trace_pdu(DIRECTION_DOWNLINK, (uint8_t *) cc->MCH_pdu.payload, TBS, module_idP, WS_M_RNTI , 0xffff,	// M_RNTI = 6 in wirehsark
		      RC.mac[module_idP]->frame,
		      RC.mac[module_idP]->subframe, 0, 0);
	    LOG_D(OPT,
		  "[eNB %d][MCH] CC_id %d Frame %d : MAC PDU with size %d\n",
		  module_idP, CC_id, frameP, TBS);
	//}

	/*
	   for (j=0;j<sdu_length_total;j++)
	   printf("%2x.",RC.mac[module_idP]->MCH_pdu.payload[j+offset]);
	   printf(" \n"); */
	return 1;
    } else {
	cc->MCH_pdu.Pdu_size = 0;
	cc->MCH_pdu.sync_area = 0;
	cc->MCH_pdu.msi_active = 0;
	cc->MCH_pdu.mcch_active = 0;
	cc->MCH_pdu.mtch_active = 0;
	// for testing purpose, fill with random data
	//for (j=0;j<(TBS-sdu_length_total-offset);j++)
	//  RC.mac[module_idP]->MCH_pdu.payload[offset+sdu_length_total+j] = (char)(taus()&0xff);
	return 0;
    }

    //this is for testing
    /*
       if (mtch_flag == 1) {
       //  LOG_D(MAC,"DUY: mch_buffer length so far is : %ld\n", &mch_buffer[sdu_length_total]-&mch_buffer[0]);
       return 1;
       }
       else
       return 0;
     */
}

MCH_PDU *get_mch_sdu(module_id_t module_idP, int CC_id, frame_t frameP,
		     sub_frame_t subframeP)
{
    //  RC.mac[module_idP]->MCH_pdu.mcs=0;
    //LOG_D(MAC," MCH_pdu.mcs is %d\n", RC.mac[module_idP]->MCH_pdu.mcs);
//#warning "MCH pdu should take the CC_id index"
    return (&RC.mac[module_idP]->common_channels[CC_id].MCH_pdu);
}

